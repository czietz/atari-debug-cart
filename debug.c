/* debug-cart: Repurpose Sidecartridge as debug output by encoding the
 * output data on the Atari address bus (on A8-A1).
 *
 * (C) 2024 by Christian Zietz <czietz@gmx.net>.
 * 
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

#define SYS_CLK 100u

#define GPIO_READ 27u
#define GPIO_ROM3 26u
#define GPIO_A1   7u

#define SM 0u
#define P pio0

#define DMA_CHAN_A 0u

#define RING_BITS 15u
#define RING_SIZE (1ul<<RING_BITS)

#define DMA_SIZE (0xFFFFFFFFul)

static uint8_t buffer[RING_SIZE] __attribute__((aligned(RING_SIZE)));

static void init_hardware(void)
{
    /* enable address bus buffer */
    gpio_init(GPIO_READ);
    gpio_set_dir(GPIO_READ, true);
    gpio_put(GPIO_READ, false);

    /* setup ROM3 signal */
    pio_gpio_init(P, GPIO_ROM3);
    gpio_set_dir(GPIO_ROM3, GPIO_IN);
    gpio_set_pulls(GPIO_ROM3, true, false); // Pull up (true, false)
    gpio_pull_up(GPIO_ROM3);
}

#define NUM_PIO_INSTR 3u

static void init_pio(void)
{
    /* assemble PIO program on-the-fly */
    uint16_t pio_asm[NUM_PIO_INSTR];
    pio_asm[0] = pio_encode_wait_gpio(1, GPIO_ROM3) | pio_encode_delay(4); // wait for ROM3 to be high + add delay to avoid glitches
    pio_asm[1] = pio_encode_wait_gpio(0, GPIO_ROM3); // wait for ROM3 to become low
    pio_asm[2] = pio_encode_in(pio_pins, 8); // sample 8 bits A8-A1
    
    /* load it */
    struct pio_program pio_prog = {
            .instructions = &pio_asm[0],
            .length = NUM_PIO_INSTR,
            .origin = -1
    };
    uint offset = pio_add_program(P, &pio_prog);

    /* configure PIO: pin base, auto-wrap, clock, FIFO autopush after 8 bits */
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_in_pins(&c, GPIO_A1);
    sm_config_set_wrap(&c, offset, offset+NUM_PIO_INSTR-1);
    sm_config_set_clkdiv(&c, 1);
    sm_config_set_in_shift(&c, true, true, 8);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    pio_sm_init(P, SM, offset, &c);
}

static void init_dma(void)
{
    /* clear the input shift counter and the FIFO */
    pio_sm_set_enabled(P, SM, false);
    pio_sm_clear_fifos(P, SM);
    pio_sm_restart(P, SM);
    
    {
        /* configure DMA channel to read from PIO FIFO and write to capture buffer */
        dma_channel_config c = dma_channel_get_default_config(DMA_CHAN_A);
        channel_config_set_read_increment(&c, false);
        channel_config_set_write_increment(&c, true);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_8); // transfer 8 bits
        channel_config_set_ring(&c, true, RING_BITS); // ring buffer on write
        channel_config_set_dreq(&c, pio_get_dreq(P, SM, false));

        dma_channel_configure(DMA_CHAN_A, &c,
            buffer,   // Destination pointer
            (uint8_t*)(&P->rxf[SM])+3,        // Source pointer 
                                // (note that 8-bit data is in the MSB of the FIFO, hence the offset
            DMA_SIZE ,          // Number of transfers: maximum possible, 4 GiB(!)
            true                // Start immediately
        );
    }
    

    /* start PIO, will wait for ROM3 transition */
    pio_sm_set_enabled(P, SM, true);
}



void main(void)
{
    uint32_t readidx = 0;
    uint32_t writeidx = 0;
    uint32_t res;
    
    /* go to 100 MHz */
    set_sys_clock_khz(SYS_CLK*1000ul, false);

    stdio_init_all();
    init_hardware();
    init_pio();
    init_dma();
    
    while (!stdio_usb_connected()) ;
    
    while (1) {

        /*
         * Calculate how much the DMA has written based on the transfer count.
         * 'writeidx' points to the byte the DMA will write _next_.
         */
        writeidx = (DMA_SIZE - dma_hw->ch[DMA_CHAN_A].transfer_count) % RING_SIZE;
        
        if (writeidx > readidx) {
            /* new data without wraparound */
            fwrite(&buffer[readidx], 1, writeidx - readidx, stdout);
            fflush(stdout);
            readidx = writeidx; // note that we assume that fwrite never fails!
        } else if (writeidx < readidx) {
            /* new data with wraparound */
            fwrite(&buffer[readidx], 1, RING_SIZE - readidx, stdout);
            fwrite(&buffer[0], 1, writeidx, stdout);
            fflush(stdout);
            readidx = writeidx; // note that we assume that fwrite never fails!
        }

    }

}
