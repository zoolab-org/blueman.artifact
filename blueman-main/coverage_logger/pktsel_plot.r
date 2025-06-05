#!/usr/bin/env Rscript

# ---------------------------------------------------
# generate_coverage_pdfs.R
#
# Usage:
#   ./generate_coverage_pdfs.R /path/to/project_dir
#
# Assumes under project_dir you have:
#   csv/       (containing field_FIXED_PROB_50.csv, field_MIXED_PROB.csv, field_RANDOM_PROB.csv)
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
  FIXED_PROB_10 = "field_FIXED_PROB_10.csv",
  FIXED_PROB_25 = "field_FIXED_PROB_25.csv",
  FIXED_PROB_50 = "field_FIXED_PROB_50.csv",
  FIXED_PROB_75 = "field_FIXED_PROB_75.csv",
  FIXED_PROB_100 = "field_FIXED_PROB_100.csv",
  SELECTIVE_75_25 = "field_SELECTIVE_75_25.csv",
  SELECTIVE_25_75 = "field_SELECTIVE_25_75.csv",
  RANDOM_PROB = "field_RANDOM_PROB.csv",
  MIXED_PROB = "field_MIXED_PROB.csv"
)

# Read each CSV into a list of data frames
data_list <- lapply(file_map, function(fname) {
  path <- file.path(input_dir, fname)
  if (!file.exists(path)) {
    # cat(sprintf("Warning: File %s does not exist. Skipping.\n", path))
    return(NULL)
  }
  else{
    cat(sprintf("Reading %s\n", fname))
    df <- read.csv(path, stringsAsFactors = FALSE)
    return(df)
  }
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
     ylim = c( min(3500, min(all_cov[all_cov > 0], na.rm = TRUE)), max(all_cov, na.rm = TRUE)),
     xlab = "Time (sec)",
     ylab = "Coverage",
     main = "Packet Selection Strategies\nFuzzing Coverage Over Time")

# Styles
line_styles <- c(FIXED_PROB_10 = 3, FIXED_PROB_25 = 3, FIXED_PROB_50 = 3,
                  FIXED_PROB_75 = 3, FIXED_PROB_100 = 3, SELECTIVE_75_25 = 2,
                  SELECTIVE_25_75 = 2, RANDOM_PROB = 4, MIXED_PROB = 1)
line_widths <- c(FIXED_PROB_10 = 1, FIXED_PROB_25 = 1.5, FIXED_PROB_50 = 2,
                  FIXED_PROB_75 = 2.5, FIXED_PROB_100 = 3, SELECTIVE_75_25 = 1,
                  SELECTIVE_25_75 = 2, RANDOM_PROB = 1, MIXED_PROB = 1)
line_colors <- c(
  FIXED_PROB_10    = '#6c8ebf',
  FIXED_PROB_25    = '#FF8000',
  FIXED_PROB_50    = '#EAC100',
  FIXED_PROB_75    = '#408080',
  FIXED_PROB_100   = '#3C3C3C',
  SELECTIVE_75_25  = '#82D900', 
  SELECTIVE_25_75  = '#7D7DFF', 
  RANDOM_PROB      = '#FF0000', 
  MIXED_PROB       = '#5B00AE'  
)
line_labels <- c(
  FIXED_PROB_10    = "Fixed (0.1)",
  FIXED_PROB_25    = "Fixed (0.25)",
  FIXED_PROB_50    = "Fixed (0.5)",
  FIXED_PROB_75    = "Fixed (0.75)",
  FIXED_PROB_100   = "Fixed (1.0)",
  SELECTIVE_75_25  = "Selective (0.75/0.25)",
  SELECTIVE_25_75  = "Selective (0.25/0.75)",
  RANDOM_PROB      = "Random",
  MIXED_PROB       = "Mixed"
)

# Plot each series
strategies <- names(data_list)  # Only those with data
for (strategy in strategies) {
  df <- data_list[[strategy]]
  
  lines(df$time_sec, df$coverage,
        type = 'l', lty = line_styles[[strategy]],
        col = line_colors[[strategy]], lwd = line_widths[[strategy]])
}

# Legend
legend("bottomright", inset = c(0.02, 0.02), xjust = 1, yjust = 0,
       legend = line_labels[strategies],
       col    = line_colors[strategies],
       lty    = line_styles[strategies],
       lwd    = line_widths[strategies],
       cex    = 0.6)

# Close PDF
invisible(dev.off())

cat(sprintf("Coverage plot saved\n"))