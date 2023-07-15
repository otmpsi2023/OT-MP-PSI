
#!/bin/bash

# Set the desired values for the number_of_parties and intersection_threshold arguments
number_of_parties=(3 4 6 8 10 12 14 16 18 20)
intersection_threshold=(2 2 3 4 5 6 7 8 9 10)

# Set the desired values for the set_size argument
set_sizes=(4 16 64 256)

# Set the output file
output_file="output/benchmark_output.txt"
> "$output_file"

# Loop over the different set sizes
for set_size in "${set_sizes[@]}"; do
    # Loop over the different number of parties and intersection thresholds
    for i in "${!number_of_parties[@]}"; do
        # Generate the configuration files for the current set of parameters
        python3 ./tools/gen_config/gen_config.py --no_print --set_size "$set_size" --number_of_parties "${number_of_parties[$i]}" --intersection_threshold "${intersection_threshold[$i]}"
        # Run the benchmark using the generated configuration files, display the output on the command line, and save it to a file
        sh ./tools/benchmark/benchmark.sh | tee -a "$output_file"
        # Pause execution for 5 seconds to let the machine do cleanup
        sleep_time=10s # adjust this value as needed
        sleep $sleep_time
    done
done