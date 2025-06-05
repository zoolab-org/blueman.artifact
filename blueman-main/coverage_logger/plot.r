#!/usr/bin/env Rscript

# ---------------------------------------------------
# generate_coverage_pdfs.R
#
# Usage:
#   ./generate_coverage_pdfs.R /path/to/project_dir
#
# Assumes under project_dir you have:
#   csv/       (containing afl.csv, field.csv, random.csv)
# Script will create:
#   cov/       (in project_dir)
# and output coverage.pdf within.
# ---------------------------------------------------

# Fetch working directory from command-line arguments
args <- commandArgs(trailingOnly = TRUE)
if (length(args) < 1) {
  stop("Usage: plot.r /path/to/project_dir")
}
project_dir <- args[1]

# Define input and output directories
input_dir  <- file.path(project_dir, "csv")
output_dir <- file.path(project_dir, "cov")

# Ensure input exists
if (!dir.exists(input_dir)) {
  stop(sprintf("Input directory '%s' does not exist", input_dir))
}

# Create output if needed
if (!dir.exists(output_dir)) {
  dir.create(output_dir)
}

# File names mapping
file_map <- list(
  FieldAware = if (file.exists(file.path(input_dir, "field.csv"))) {
    "field.csv"
  } else if (file.exists(file.path(input_dir, "field_FIXED_PROB_50.csv"))) {
    "field_FIXED_PROB_50.csv"
  } else {
    NULL
  },
  Random     = "random.csv",
  AFLonly    = "afl.csv"
)
file_map <- Filter(Negate(is.null), file_map)

# Read each CSV into a list of data frames
data_list <- lapply(file_map, function(fname) {
  path <- file.path(input_dir, fname)
  if (!file.exists(path)) {
    # stop(sprintf("Expected file '%s' not found", path))
    return(NULL)
  }
  cat(sprintf("Reading %s\n", fname))
  df <- read.csv(path, stringsAsFactors = FALSE)
  return(df)
})
names(data_list) <- names(file_map)
data_list <- Filter(Negate(is.null), data_list)

# Setup PDF device
out_pdf <- file.path(output_dir, "coverage.pdf")
pdf(out_pdf, width = 4, height = 4)

# Determine axis limits
all_times <- unlist(lapply(data_list, function(df) df$time_sec))
all_cov   <- unlist(lapply(data_list, function(df) df$coverage))

# Initialize plot
plot(NA, NA,
     xlim = c(0, max(all_times, na.rm = TRUE)),
     ylim = c(0, max(all_cov, na.rm = TRUE)),
     xlab = "Time (sec)",
     ylab = "Coverage",
     main = "Fuzzing Coverage Over Time")

# Styles
line_styles <- c(FieldAware = 1, Random = 3, AFLonly = 2)
line_colors <- c(FieldAware = '#6c8ebf', Random = '#b85450', AFLonly = '#579857')

# Plot each series
strategies <- names(data_list)  # Only those with data
for (strategy in strategies) {
  df <- data_list[[strategy]]
  lines(df$time_sec, df$coverage,
        type = 'l', lty = line_styles[[strategy]],
        col = line_colors[[strategy]], lwd = 2)
}

# Legend
# legend(355, 2300,
legend("bottomright", inset = c(0.02, 0.02), xjust = 1, yjust = 0,
       legend = names(data_list),
       col    = line_colors[strategies],
       lty    = line_styles[strategies],
       lwd    = 2,
       cex    = 0.6)

# Close PDF
# dev.off()
invisible(dev.off())

cat(sprintf("Coverage plot saved\n"))

