import subprocess
import sys

error_count = 0;
error_score = 0;
str_prefix = "Total errors found: "

output_file = sys.argv[1]
basic_score = sys.argv[2]

with open(output_file) as f:
    for line in f:
        if line.startswith(str_prefix):
            print(line)
            error_score = 20
            error_count = line[len(str_prefix):]
            error_score = error_score - 1 * int(error_count)
            error_score = error_score + int(basic_score)

            if error_score < 0:
                error_score = 0
            elif error_score >= 20:
                error_score = 20

sys.exit(error_score)