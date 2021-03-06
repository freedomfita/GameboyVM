#include <stdio.h>
#include <iostream>
using std::cout;
using std::endl;


#include <string.h>
#include <string>
using std::string;

#define TIMER 0xFF05
#define TIMER_MODULATOR 0xFF06
#define TIMER_CONTROLLER 0xFF07

#define FLAG_Z 7;
#define FLAG_N 6;
#define FLAG_H 5;
#define FLAG_C 4;

//Timer controller has 4 frequencies to set
//the timer to count up at
//4096, 262144, 65536, 16384 Hz
//CPU clock speed runs at 4194304 Hz

#define CLOCKSPEED 4194304 ;
enum color_t {WHITE=0, LIGHT_GRAY=1, DARK_GRAY=2, BLACK=3};

typedef unsigned char BYTE;
typedef char SIGNED_BYTE;
typedef unsigned short WORD;
typedef signed short SIGNED_WORD;

union Register
{
    WORD reg;
    struct
    {
        BYTE low;
        BYTE high;
    };
};


class GB
{
public:
    //Constructor
    GB();
    void update();
    int get_opcode();
    void update_timers(int cycles);
    void update_graphics(int cycles);
    void check_interrupts();
    void draw_screen();
    void write_address(WORD address, BYTE data);
    void set_MBCs(int value);
    void handle_banking(WORD address, BYTE data);
    BYTE read_memory(WORD address) const;
    void enable_ram_bank(WORD address, BYTE data);
    void change_low_rom_bank(BYTE data);
    void change_high_rom_bank(BYTE data);
    void change_ram_bank(BYTE data);
    void change_rom_ram_mode(BYTE data);
    bool test_bit(WORD address, int bit) const;
    int get_bit(BYTE byte, int bit) const;
    void divide_register(int cycles);
    bool is_clock_enabled() const;
    void set_clock_frequency();
    void request_interrupt(int interrupt);
    BYTE get_clock_frequency() const;
    BYTE set_bit(BYTE addr, int position);
    void service_interrupt(int interrupt);
    void push_word_on_stack(WORD word);
    BYTE reset_bit(BYTE addr, int position);
    void set_LCD_status();
    bool is_LCD_enabled() const;
    void draw_scanline();
    void do_DMA_transfer(BYTE data);
    void render_sprites();
    void render_tiles();
    BYTE get_lcd_control_register();
    color_t get_color(BYTE color_num, WORD address) const;


private:
    BYTE cartridge_memory[0x200000];
    BYTE screen_data[160][144][3];
    BYTE current_ROM_bank;
    BYTE current_RAM_bank;
    BYTE ram_banks[0x8000];
    int current_frequency;// = 4096;
    //timer_counter = CLOCK_SPEED / current_frequency
    int timer_counter;// = 1024;
    int divider_counter;
    int divider_register;
    int m_scanline_counter;

    bool MBC1;
    bool MBC2;
    bool enable_ram;
    bool rom_banking;
    bool master_interrupt;

    Register regAF;
    Register regBC;
    Register regDE;
    Register regHL;


    WORD program_counter;
    Register stack_pointer;
    BYTE rom_mem[0x10000];
};
