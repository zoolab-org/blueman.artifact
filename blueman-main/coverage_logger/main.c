#include "coverage_logger.h"
#include <unistd.h>

int main() {
    init_coverage_logger("my_coverage_log1.json");
    setup_sigquit_handler();

    for (int i = 0; i < 11000; ++i) {
        add_coverage_entry(i * 0.01, i);
        usleep(1000);  // 1ms

        // Press Ctrl+\ to trigger SIGQUIT and save
    }

    save_coverage_to_file();  // Optional if not interrupted
    return 0;
}
