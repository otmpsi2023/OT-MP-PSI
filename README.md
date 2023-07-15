# Practical Over-threshold Multiparty Private Set Intersection

This repository contains experimental code for "Practical Over-threshold Multiparty Private Set Intersection with Fast Online Execution". 

## Table of Contents

- [About The Repo](#about-the-repo)
- [Dependencies](#dependencies)
- [Contents](#contents)
  - [OTMPSI](#otmpsi)
  - [Online-enhanced OTMPSI](#online-enhanced-otmpsi)
  - [TIFS](#tifs)
- [Contact](#contact)

## About The Repo

We have successfully compiled the code on Ubuntu 22, macOS Monterey (Intel chip) and macOS Sonoma (Apple Silicon). Please note that the implementation is for experimental usage only.

## Dependencies

This project has the following dependencies:

- [BOOST](https://www.boost.org/) (version 1.81)
- [GMP](https://gmplib.org/) (version 6.2.1)
- [NTL](https://libntl.org/) (version 11.5.1)

Please make sure to install these dependencies before building and running the project.

## Contents

### OTMPSI

OTMPSI implementation, for the formal description and analysis of the protocol, please refer to the paper.

### Online-enhanced OTMPSI

Online-enhanced OTMPSI implementation. Last several steps of the standard version are modified to boost the online performance, but the price is that the overall performance is worse.

### TIFS21

To better compare run-time results with Bay et al.'s protocol, we tried to integrate their implementation into the same schema as ours. 

## Contact

- Huiyang He: [hhe@mail.ustc.edu.cn](mailto:hhe@mail.ustc.edu.cn)
- Le Yang: [leyang@mail.ustc.edu.cn](mailto:leyang@mail.ustc.edu.cn)
