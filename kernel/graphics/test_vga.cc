#include "vga.h"
#include "debug.h"

// Test function to demonstrate VGA functionality
void test_vga_standalone() {
    Debug::printf("Running standalone VGA test...\n");
    
    // Initialize VGA
    vga_init();
    
    // Clear the screen
    vga_clear_screen();
    
    // Test basic pixel plotting
    for (int i = 0; i < 320; i += 10) {
        vga_plot_pixel(i, 0, 15);  // White pixels at top
    }
    
    for (int i = 0; i < 200; i += 10) {
        vga_plot_pixel(0, i, 4);   // Red pixels at left
    }
    
    // Test drawing a rectangle
    for (int x = 100; x < 200; x++) {
        for (int y = 50; y < 100; y++) {
            vga_plot_pixel(x, y, 2);  // Green rectangle
        }
    }
    
    // Test palette setting
    vga_set_palette(5, 63, 0, 0);    // Bright red
    vga_set_palette(6, 0, 63, 0);    // Bright green
    vga_set_palette(7, 0, 0, 63);    // Bright blue
    
    // Draw some pixels with our custom palette colors
    for (int i = 0; i < 100; i++) {
        vga_plot_pixel(50 + i, 150, 5);  // Red line
        vga_plot_pixel(50 + i, 160, 6);  // Green line
        vga_plot_pixel(50 + i, 170, 7);  // Blue line
    }
    
    // Test mode setting
    vga_set_mode(VGA_MODE_GRAPHICS);
    
    // Draw a border around the screen
    for (int i = 0; i < 320; i++) {
        vga_plot_pixel(i, 0, 15);       // Top border
        vga_plot_pixel(i, 199, 15);     // Bottom border
    }
    for (int i = 0; i < 200; i++) {
        vga_plot_pixel(0, i, 15);       // Left border
        vga_plot_pixel(319, i, 15);     // Right border
    }
    
    Debug::printf("Standalone VGA test completed.\n");
}

// Main function to run the test
extern "C" void test_vga_main() {
    test_vga_standalone();
    
    // Keep the screen visible
    while (1) {
        // Do nothing, just keep the screen visible
    }
} 