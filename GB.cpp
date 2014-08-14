#include <iostream>
#include <string.h>
#include "GB.h"


// Notes
// ROM bank 0 is fixed from 0x0000 to 0x3FFF, so any other loaded
// ROM banks will be in 0x4000 to 0x7FFF

GB::GB()
{
    std::cout << "About to load game\n";
    memset(cartridge_memory, 0, sizeof(cartridge_memory));
    FILE *game_file;
    game_file = fopen( "SuperMarioLand.gb", "rb" );
    fread(cartridge_memory, 1, 0x200000, game_file);
    fclose(game_file);
    std::cout << "Finished Loading game\n";
    stack_pointer.reg=0xFFFE;

    current_frequency = 4096;
    timer_counter = 1024;

    rom_mem[0xFF05] = 0x00;
    rom_mem[0xFF06] = 0x00;
    rom_mem[0xFF07] = 0x00;
    rom_mem[0xFF10] = 0x80;
    rom_mem[0xFF11] = 0xBF;
    rom_mem[0xFF12] = 0xF3;
    rom_mem[0xFF14] = 0xBF;
    rom_mem[0xFF16] = 0x3F;
    rom_mem[0xFF17] = 0x00;
    rom_mem[0xFF19] = 0xBF;
    rom_mem[0xFF1A] = 0x7F;
    rom_mem[0xFF1B] = 0xFF;
    rom_mem[0xFF1C] = 0x9F;
    rom_mem[0xFF1E] = 0xBF;
    rom_mem[0xFF20] = 0xFF;
    rom_mem[0xFF21] = 0x00;
    rom_mem[0xFF22] = 0x00;
    rom_mem[0xFF23] = 0xBF;
    rom_mem[0xFF24] = 0x77;
    rom_mem[0xFF25] = 0xF3;
    rom_mem[0xFF26] = 0xF1;
    rom_mem[0xFF40] = 0x91;
    rom_mem[0xFF42] = 0x00;
    rom_mem[0xFF43] = 0x00;
    rom_mem[0xFF45] = 0x00;
    rom_mem[0xFF47] = 0xFC;
    rom_mem[0xFF48] = 0xFF;
    rom_mem[0xFF49] = 0xFF;
    rom_mem[0xFF4A] = 0x00;
    rom_mem[0xFF4B] = 0x00;
    rom_mem[0xFFFF] = 0x00;
    program_counter = 0x100;
    //Which rom bank is loaded. not 0 because bank 0 is always present
    //Rom banking not used in MBC2
    current_ROM_bank = 1;
    
    memset(&ram_banks,0,sizeof(ram_banks));
    current_RAM_bank = 0;
    enable_ram = false;
}

//Pass in cartridge_memory 0x147, which tells us if MBC1 or MBC2 are used
//then store this 
void GB::set_MBCs(int value)
{
    switch( value )
    {
        case 1: MBC1 = true; break;
        case 2: MBC1 = true; break;
        case 3: MBC1 = true; break;
        case 5: MBC2 = true; break;
        case 6: MBC2 = true; break;
        default : break;
    }
}

// read memory can't modify member variables, set to const
// 0x4000 - 0x7FFF is our rom mem bank
// 0xA000 - 0xBFFF is our ram mem ba nk
BYTE GB::read_memory(WORD address) const
{
    // are we reading from the rom memory bank
    if ((address>=0x4000) && (address <= 0x7FFF))
    {
        WORD new_address = address - 0x4000;
        return cartridge_memory[new_address + (current_ROM_bank * 0x4000)];
    }
    //ram memory bank
    else if ((address >= 0xA000) && (address <= 0xBFFF))
    {
        WORD new_address = address - 0xA000;
        return ram_banks[new_address + (current_RAM_bank*0x2000)];
    }

    //return memory if not ram or rom
    return rom_mem[address];
}

// Game update cycle. GB renders screen every 69905 instructions,
// so we shoot for that. Every instruction returns its number of cycles
// so we know the total cycles GB expects
void GB::update()
{
    const int MAX_CYCLES = 69905;
    int current_cycles = 0;
    while (current_cycles < MAX_CYCLES)
    {
        int cycles = get_opcode();
        current_cycles += cycles;
        update_timers(cycles);
        update_graphics(cycles);
        check_interrupts();
    }
    draw_screen();

}

// Overview
// GB tries to write to rom, so we intercept, and try to figure out
// why it's writing. We check the memory address of where it wants to
// write to ROM, and take certain actions based on that.
// 0x4000 - 0x6000 RAM bank change or ROM bank change, depending on 
// what current rom/ram mode is enabled.
// 0x0000 - 0x2000 Enables RAM bank writing
void GB::write_address(WORD address, BYTE data)
{
    // ROM bank memory
    if (address < 0x8000)
    {
        // determine banking 
        handle_banking(address, data);
    }
    else if ( (address >= 0xA000 ) && (address < 0xC000) )
    {
        //check if ram is enabled. then put data in address.
        if (enable_ram)
        {
            //since address is overall, but our banks are separate
            //subtract base address, then put data in correct spot
            //in ram_banks
            WORD new_address = address - 0xA000;
            ram_banks[new_address + (current_RAM_bank * 0x2000)] = data;
        }
    }
    else if ( (address >= 0xE000 ) && (address < 0xFE00) )
    {
        rom_mem[address] = data;
        write_address(address - 0x2000, data);
    } // sprite attribute table
    else if ( ( address >= 0xFEA0 ) && (address < 0xFEFF) )
    {
    } //0xFF00 - 0xFF7F device mappings. used to access I/O devices
      //0xFF80-0xFFFE High RAM Area
      //0xFFFF Interrupt Enable Register
    else if (TIMER_CONTROLLER == address)
    {
        BYTE current_freq = get_clock_frequency(); 
        //GAME MEMORY?? not defined..
        //game_memory[TIMER_CONTROLLER] = data;
        rom_mem[TIMER_CONTROLLER] = data;
        BYTE new_frequency = get_clock_frequency();
        
        if (current_freq != new_frequency)
        {
            set_clock_frequency();
        }

    }
    //trap divide register, baby
    else if (0xFF04 == address)
    {
        rom_mem[0xFF04] = 0;
    }
    else if (address == 0xFF44)
    {
        rom_mem[address] = 0;
    }
    else if (address == 0xFF46)
    {
            do_DMA_transfer(data);
    }
    else
    {
        rom_mem[address] = data;
    }
}

//clock frequency is a combo of bit 1 and 0 of TIMER_CONTROLLER
BYTE GB::get_clock_frequency() const
{
    return read_memory(TIMER_CONTROLLER) & 0x3;
}

//Reset timer counter once it reaches zero to the correct value
//so it can continue counting down at the same rate
void GB::set_clock_frequency()
{
    BYTE frequency = get_clock_frequency();
    switch (frequency)
    {
        case 0: timer_counter = 1024; break; //4096
        case 1: timer_counter = 16; break;  //262144
        case 2: timer_counter = 64; break;  //65536
        case 3: timer_counter = 256; break; //16382
    }
}

void GB::handle_banking(WORD address, BYTE data)
{
    // RAM enabling
    if (address < 0x2000)
    {
        if (MBC1 || MBC2)
        {   
            //No ram banking without MBC1 or 2
            enable_ram_bank(address, data);
        }
    }
    // low ROM bank change
    else if ((address >= 0x2000) && (address < 0x4000))
    {
        if (MBC1 || MBC2)
        {
            change_low_rom_bank(data);
        }
    }
    else if ((address >= 0x4000) && (address < 0x6000))
    {
        // there is no rambank in mbc2 always use rambank 0
        if (MBC1)
        {
            if (rom_banking)
            {
                //High rom bank change
                change_high_rom_bank(data);
            }
            else
            {
                // Change ram bank
                change_ram_bank(data);
            }
        }
    }
    // ROM banking or RAM banking bro?
    else if ((address >= 0x6000) && (address < 0x8000))
    {
        if (MBC1)
        {
            change_rom_ram_mode(data);
        }
    }

}


void GB::change_rom_ram_mode(BYTE data)
{
    BYTE new_data = data & 0x1;
    rom_banking = (new_data == 0) ? true : false;
    if (rom_banking)
        current_RAM_bank = 0;
}

void GB::change_ram_bank(BYTE data)
{
    current_RAM_bank = data & 0x3;
}

void GB::change_high_rom_bank(BYTE data)
{
    //turn off upper 3 bits of the current rom
    current_ROM_bank &= 31;

    //turn off the lower 5 bits of the data
    data &= 224;
    current_ROM_bank |= data;
    if (current_ROM_bank == 0) current_ROM_bank++;
}

bool GB::test_bit(WORD address, int bit) const
{
    return ((address) & (1 << bit ));
}

void GB::change_low_rom_bank(BYTE data)
{
    if (MBC2)
    {
        current_ROM_bank = data & 0xF;
        if (current_ROM_bank == 0) current_ROM_bank++;
        return;
    }
    BYTE lower_5 = data & 31;
    current_ROM_bank &= 224; // turn off the lower 5
    current_ROM_bank |= lower_5;
    if (current_ROM_bank == 0) current_ROM_bank++;
}

//Overview
//A game must request that ram banking writing is enabled in order to
//write to a RAM bank. 

void GB::enable_ram_bank(WORD address, BYTE data)
{
    if (MBC2)
    {
        if (test_bit(address, 4) == 1) return;
    }
    BYTE test_data = data & 0xF;
    if (test_data == 0xA)
        enable_ram = true;
    else if (test_data == 0x0)
        enable_ram = false;
}

void GB::check_interrupts()
{
    if (master_interrupt == true)
    {
        BYTE req = read_memory(0xFF0F);
        BYTE enabled = read_memory(0xFFFF);
        if (req > 0)
        {
            for (int i = 0; i < 5; i++) //just 5 possibilties
            {
                if (test_bit(req,i) == true)
                {
                    if (test_bit(enabled, i))
                        service_interrupt(i);
                }
            }
        }
    }
}

void GB::service_interrupt(int interrupt)
{
    //interrupt routines
    //V-Blank: 0x40
    //LCD: 0x48
    //TIMER: 0x50
    //JOYPAD: 0x60
    master_interrupt = false;
    BYTE req = read_memory(0xFF0F);
    req = reset_bit(req, interrupt);
    write_address(0xFF0F,req);

    //save current execution
    push_word_on_stack(program_counter);

    switch(interrupt)
    {
        case 0: program_counter = 0x40; break;
        case 1: program_counter = 0x48; break;
        case 2: program_counter = 0x50; break;
        case 3: program_counter = 0x60; break;
    }
}

void GB::push_word_on_stack(WORD word)
{

}

BYTE GB::reset_bit(BYTE addr, int position)
{
    addr &= ~(1 << position);
    return addr;
}

bool GB::is_LCD_enabled() const{
    return test_bit(read_memory(0xFF40),7);
}

void GB::draw_scanline() {

}

void GB::set_LCD_status() {
    BYTE status = read_memory(0xFF41);
    if (false == is_LCD_enabled())
    {
        // set mode to 1 during lcd disabled, and reset scanline
        m_scanline_counter = 456;
        rom_mem[0xFF44] = 0;
        status &= 252;
        status = set_bit(status, 0);
        write_address(0xFF41, status);
        return;
    }
    
    BYTE current_line = read_memory(0xFF44);
    BYTE current_mode = status & 0x3;

    BYTE mode = 0;
    bool req_int = false;

    // vblank mode, so set mode to 1
    if (current_line >= 144)
    {
        mode = 1;
        status = set_bit(status,0);
        status = reset_bit(status, 1);
        req_int = test_bit(status, 4);
    }
    else
    {
        int mode2_bounds = 456-80;
        int mode3_bounds = mode2_bounds - 172;

        // mode 2
        if (m_scanline_counter >= mode2_bounds)
        {
            status = set_bit(status, 1);
            status = reset_bit(status, 0);
            req_int = test_bit(status, 5);
        }
        // mode 3
        else if (m_scanline_counter >= mode3_bounds)
        {
            mode = 3;
            status = set_bit(status,1);
            status = set_bit(status,0);
        }
        else 
        {
            mode = 0;
            status = reset_bit(status,1);
            status = reset_bit(status,0);
            req_int = test_bit(status,3);
        }

        // just entered a new mode, request interrupt
        if (req_int && (mode != current_mode))
            request_interrupt(1);

        if (current_line == read_memory(0xFF45))
        {
            status = set_bit(status,2);
            if (test_bit(status,6))
                request_interrupt(1);
        }
        
        else
        {
            status = reset_bit(status, 2);
        }
    
        write_address(0xFF41, status);
    }
}

void GB::do_DMA_transfer(BYTE data)
{
    WORD address = data << 8; // source address is data * 100
    for (int i = 0; i < 0xA0; i++)
    {
        write_address(0xFE00+i, read_memory(address+i));
    }
}


// 0-153 visible scanlines
void GB::update_graphics(int cycles)
{
    //do something
    set_LCD_status();
    if (is_LCD_enabled())
        m_scanline_counter -= cycles;
    else
        return;
    
    if (m_scanline_counter <= 0)
    {
        // move to next scanline
        rom_mem[0xFF44]++;
        BYTE current_line = read_memory(0xFF44);

        m_scanline_counter = 456;

        // vertical blank period
        if (current_line == 144)
            request_interrupt(0);

        //if past scanline 153, reset to 0
        else if (current_line > 153)    
            rom_mem[0xFF44] = 0;

        else if (current_line < 144)
            draw_scanline();

    }
}
void GB::update_timers(int cycles)
{
    divide_register(cycles);
    //enable the clock in order to update it
    if (is_clock_enabled())
    {
        timer_counter -= cycles;
        
        //Enough clock cycles to update the timers
        if (timer_counter <= 0)
        {
            set_clock_frequency();
            //timer almost overflowing
            if (read_memory(TIMER) == 255)
            {
                write_address(TIMER, read_memory(TIMER_MODULATOR));
                //timer interrupt is bit 2 of interrupt request register
                request_interrupt(2);
            }
            else
            {
                write_address(TIMER, read_memory(TIMER) + 1);
            }
        }
    }
}

BYTE GB::set_bit(BYTE addr, int position)
{
    addr |= 1 << position;
    return addr;
}

void GB::request_interrupt(int interrupt)
{
    BYTE req = read_memory(0xFF0F);
    req = set_bit(req, interrupt);
    write_address(0xFF0F, interrupt);
}



//check bit 2 to see if timer is enabled
bool GB::is_clock_enabled() const
{
    return test_bit(read_memory(TIMER_CONTROLLER), 2) ? true : false;
}

void GB::divide_register(int cycles)
{
    divider_register += cycles;
    if (divider_counter >= 255)
    {
        divider_counter = 0;
        rom_mem[0xFF04]++;
    }
}

int GB::get_opcode()
{
    return 4;
}

void GB::draw_screen()
{
    //do something
}



int main() 
{
    std::cout << "Hello World!\n";
    GB gb;
    return 0;
}



