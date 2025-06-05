#ifndef COVERAGE_LOGGER_H
#define COVERAGE_LOGGER_H

void init_coverage_logger(const char *filename);
void add_coverage_entry(uint64_t time, int coverage);
void save_coverage_to_file();

#endif // COVERAGE_LOGGER_H
