<!-- Improved compatibility of back to top link: See: https://github.com/othneildrew/Best-README-Template/pull/73 -->
<a name="readme-top"></a>
<!--
*** Thanks for checking out the Best-README-Template. If you have a suggestion
*** that would make this better, please fork the repo and create a pull request
*** or simply open an issue with the tag "enhancement".
*** Don't forget to give the project a star!
*** Thanks again! Now go create something AMAZING! :D
-->




## Table of Contents

- [Dependency](#dependency)
- [Getting Started](#getting-started)
    - [Prerequisites](#prerequisites)
        - [Linux](#linux)
        - [macOS](#macos)
    - [Installation](#installation)
- [Usage](#usage)
    - [gen_prime](#gen_prime)
        - [Arguments](#arguments)
    - [gen_config.py](#gen_configpy)
        - [Arguments](#arguments-1)
        - [Usage](#usage-1)
    - [main](#main)
    - [benchmark](#benchmark)
    - [run_benchmark.sh](#run_benchmarksh)
- [Known Bugs](#known-bugs)
- [Contact](#contact)

## Dependency

This project has the following dependencies:

- [BOOST](https://www.boost.org/) (version 1.81)
- [GMP](https://gmplib.org/) (version 6.2.1)
- [NTL](https://libntl.org/) (version 11.5.1)

Please make sure to install these dependencies before building and running the project.



<!-- GETTING STARTED -->

## Getting Started

These instructions will help you get a copy of the project up and running on your local machine for development and
testing purposes.

### Prerequisites

Before you begin, make sure you have installed all the dependencies listed in the [Dependencies](#dependencies) section.

#### Linux:

On Linux, you can install the dependencies by following these steps:

* Install Boost:
   ```
   % wget -O boost_1_81_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.81.0/boost_1_81_0.tar.gz/download
   % tar xzvf boost_1_81_0.tar.gz
   % cd boost_1_81_0/
   % sudo apt-get install build-essential autoconf
   % ./bootstrap.sh --prefix=/usr/
   % ./b2
   % sudo ./b2 install
   ```
* Install GMP:
   ```
    % wget https://gmplib.org/download/gmp/gmp-6.2.1.tar.xz
    % tar -xvf gmp-6.2.1.tar.xz
    % cd gmp-6.2.1
    % ./configure
    % make
    % make check
    % sudo make install
   ```
* Install NTL:
   ```
    % wget https://libntl.org/ntl-11.5.1.tar.gz
    % gunzip ntl-11.5.1.tar.gz
    % tar xf ntl-11.5.1.tar
    % cd ntl-11.5.1/src
    % ./configure 
    % make
    % make check
    % sudo make install
   ```

#### macOS:

On macOS, we suggest using [Homebrew](https://brew.sh/) to install all the dependencies:

```
   brew install boost gmp ntl
```
Please note that you may need to update the library path in the makefile to match the location where Homebrew installed the dependencies.

If you encounter any issues with installing the dependencies using Homebrew, you can try following the Linux installation steps provided above.

### Installation

1. Clone this repository to your local machine:
   ```sh
   git clone https://github.com/otmpsi2023/Otmpsi.git
   ```
2. cd *project-directory*
3. Build the project
   ```sh
   make clean
   make all
   ```

<!-- USAGE EXAMPLES -->

## Usage

After compiling the project, you will find three executable files under the `/bin` directory: `gen_prime`, `main`,
and `benchmark`. There is also a python script `gen_config.py` under `/tools/gen_config`.

In general, you need to first use `gen_prime` to search for qualified encryption parameters. Then, use `gen_config` to
generate a list of configuration files. These files will be used by either `main` or `benchmark`. Note
that `gen_config.py` already contains some default parameters for sample tests.

### gen_prime

`gen_prime` is used to generate a large prime number `p` for ElGamal encryption. Our implementation of ElGamal
encryption requires `p` to have some special properties. For more details, please refer to our paper.

#### Arguments

The script accepts the following command line arguments:

- `-sec`: The number of security bits (default: 2048)
- `-q`: The q value (default: 2)
- `-power`: The power of q (default: 75)

### gen_config.py

This is a Python script that uses the argparse module to parse command line arguments.

#### Arguments

The script accepts the following arguments:

- `--set_size`: The size of the set (default: 2^8)
- `--false_positive_rate`: The false positive rate (default: 0.001)
- `--number_of_parties`: The number of parties (default: 10)
- `--intersection_threshold`: The intersection threshold (default: 5)
- `--benchmark_rounds`: The number of benchmark rounds (default: 50)
- `--number_of_hash_functions`: The number of hash functions (default: 11)
- `--server_port`: The server port starts from (default: 20081)
- `-p` or `--p`: The p value (default: see source code for details)
- `--p_bits`: The number of bits in p (default: 2176)
- `--prime_factor_1`: The first prime factor (default: see source code for details)
- `--prime_factor_2`: The second prime factor (default: see source code for details)
- `-q` or `--q`: The q value (default: 2)
- `--q_power`: The power of q (default: 76)

To use the script, run it with the desired arguments. For example:

```
python3 ./tools/gen_config/gen_config.py --set_size 256 --false_positive_rate 0.01
```

This will run the script with a set size of 256 and a false positive rate of 0.01, and produce several JSON files under
the `./config` directory

### main

`main` is the executable for single-time experimental execution.

To run the experiment on your machine, first generate configuration files and then run a shell script `run.sh` under
the `./tools` directory. Run the script to start the experiment:

```
sh ./tools/run.sh
```

### benchmark

To run the benchmark, first generate the configuration files as explained above. Then, run the following command:

```
sh ./tools/benchmark/benchmark.sh
```

### run_benchmark.sh

This script automates the process of running multiple benchmarks with different parameter sets. It generates configuration files using the `gen_config.py` script for different combinations of `set_size`, `number_of_parties`, and `intersection_threshold` values, and then runs the `benchmark` component using those configuration files. The output of each benchmark run is appended to `output/benchmark_output.txt`. The script also includes a 5-second pause between benchmarks to allow the machine to do any necessary cleanup.

```
bash ./tools/benchmark/run_benchmark.sh
```

<!-- LICENSE -->
<!-- ## License

Distributed under the MIT License. See `LICENSE.txt` for more information. -->

<!-- BUGS -->

## Known Bugs

Here is a list of known bugs that we have encountered while developing and testing this project:

- Boost Asio shared_pointer != 0: This issue has occurred on Ubuntu but not on macOS. We have not found a solution yet, but restarting the program seems to solve the problem.

```
typename boost::detail::sp_member_access<T>::type boost::shared_ptr<T>::operator->() const [with T = TcpChannel; typename boost::detail::sp_member_access<T>::type = TcpChannel*]: Assertion `px != 0' failed.
```

- Finish without output: This issue occurred only once during whole benchmarking but we were unable to reproduce it.




<!-- ACKNOWLEDGMENTS -->
<!-- ## Acknowledgments

* []()
* []()
* []() -->




<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->

[Ntl-url]: https://libntl.org/

[Gmp-url]: https://gmplib.org/

[Boost-url]: https://www.boost.org/


