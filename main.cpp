/*
 * main.cpp
 *
 *  Created on: 1 ��� 2023 �.
 *      Author: semyo
 */
#include <iostream>
#include <fstream>
#include <random>
#include <cstring>
#include <string>
#include <stdexcept>
#include <cerrno>
#include <libgen.h>
#include <functional>
#include <algorithm>
#include <vector>
#include <cassert>
#include <utility>
#include <string_view>
#include "headers.h"

using namespace std;
typedef unsigned long long ull;
#pragma pack(push, 1)
struct CryptoContainerHeader
{
	uint64_t orig_size;
	uint32_t orig_name_length;
	uint32_t block_size;

};
#pragma pack(pop)


void keyForm(std::vector <unsigned int> &Key) {
  Key.resize(33);
  for (size_t i = 1; i <= 8; i++) {
    Key[i+8]  = Key[i];
    Key[i+16] = Key[i];
    Key[i+24] = Key[9 - i];
  }
}

bool there_is(const std::string flag, unsigned char &pos) {
  for (size_t i = 0; i < flags.size(); i++) {
    if (flags[i] == flag) {
      pos = i;
      return true;
    }
  }
  return false;
}

void c_fl_fill(int argc, char *argv[], std::vector <char> &c_fl) {
  unsigned char pos = 0;
  for (int i = 1; i < argc; i++) {
    if (there_is(argv[i], pos)) {
      c_fl[pos] += flag_val[pos];
    }
  }
}

void check_number_flags(int argc, char *argv[], const std::vector <char> &c_fl) {
  const unsigned char modes_sum = c_fl[2] + c_fl[3] + c_fl[4] + c_fl[5] + c_fl[6] + c_fl[7]; // Сумма количества встречающийхся режимов,
  const unsigned char help_sum = c_fl[0] + c_fl[1];                                          // флагов на помощь,
  unsigned char count_flags_sum = 0;                                                   // всех флагов
  for (size_t i = 0; i < 14; i++) {
    count_flags_sum += c_fl[i];
  }
  if (count_flags_sum != argc-1) {
    throw "Wrong number of arguments!\n";
  }
  if (help_sum > 1) {
    throw "Use -h or --help! Not both!\n";
  } else if (((modes_sum + help_sum) == 0) || ((modes_sum + help_sum) > 1)) {
    throw "Wrong mode!\n";
  } else if ((c_fl[8] + c_fl[9]) > 1) {
    throw "You can not encrypt and decrypt at the same time!\n";
  }
}

void check_file_flag_pos(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if ((argv[i] == flags[10] || argv[i] == flags[11] || argv[i] == flags[12] || argv[i] == flags[13]) && (i == argc-1)) {
      throw "File path must be after file flag!\n";
    }
  }
}

void is_there_k(int argc, char *argv[]) {
  bool k_flag = true;
  for (int i = 1; i < argc; i++) {
    if (argv[i] == flags[10]) {
      k_flag = false;
    }
  }
  if (k_flag) {
    throw "No key found!\n";
  }
}

void check_enc_dec(int argc, char *argv[], const std::vector <char> &c_fl) {
  if (((c_fl[8] + c_fl[9]) == 0) && (!c_fl[7])) {
    throw "�������� ����� enc ��� dec!\n";
  } else if ((c_fl[7] + c_fl[9] + c_fl[8]) > 1) {
    throw "Mac mode has no encryption and decryption modes!\n";
  } else if ((c_fl[2] + c_fl[7] + c_fl[11]) > 3) {
    throw "IV not needed in ecb or mac modes!\n";
  }
}

void check_args(int argc, char *argv[], std::vector <char> &c_fl) {
  c_fl_fill(argc, argv, c_fl);
  check_number_flags(argc, argv, c_fl);
  check_file_flag_pos(argc, argv);
  is_there_k(argc, argv);
  check_enc_dec(argc, argv, c_fl);
}

unsigned int t(const unsigned int& a) {
  unsigned int subVal[8];     //������ ��� ���������� ����������� ����������� ��������������
  unsigned int highligh = 15; //�������� �������� 4 ����
  for (size_t i = 0; i < 8; i++) {      //��������� ������ �� 4 ����
    subVal[i] = (a & highligh) >> i * 4;
    highligh = highligh << 4;
  }
  for (size_t i = 0; i < 8; i++) { //���������� ���������� ��������������
    unsigned int tmp = subVal[i];
    subVal[i] = pi[i][tmp];
  }
  unsigned int ans = 0;
  for (size_t i = 0; i < 8; i++) { //������������ ������ �� 4 �����
    ans += (subVal[i] << (4 * i));
  }
  return ans;
}

unsigned int g(const unsigned int& key, const unsigned int& a0) {
  unsigned int Sum = t(key + a0);
  return ((Sum << 11) + ((Sum & (2047 << 21)) >> 21));  //������ ����������� ����� ����� �� 11
}

void G(const unsigned int& key, unsigned int& a1, unsigned int& a0) {
  unsigned long A1 = a1;  //��������� ������ �������� �1
  a1 = a0;
  a0 = g(key, a0) ^ A1;
}

unsigned long long Gl(const unsigned int& key, unsigned int& a1, unsigned int& a0) {
  return ((( ((unsigned long long)g(key, a0)) ^ a1) << 32) + a0);
}

unsigned long long enc(const std::vector <unsigned int>& key, unsigned int& a1, unsigned int& a0) {
  for (size_t j = 1; j < 32; j++) {
    G(key[j], a1, a0);
  }
  return Gl(key[32], a1, a0);
}

unsigned long long dec(const std::vector <unsigned int>& key, unsigned int& a1, unsigned int& a0) {
  for (size_t j = 32; j > 1; j--) {
    G(key[j], a1, a0);
  }
  return Gl(key[1], a1, a0);
}

unsigned int search(char* argv[], const std::string &flag, int argc) {
  for (int i = 1; i < argc; i++) {
    if (argv[i] == flag) {
      return i+1;
    }
  }
  return 0;
}

unsigned int file_check(std::ifstream &in) {
  in.seekg(0, in.end);
  unsigned int length = in.tellg();
  in.seekg(0, in.beg);
  return length;
}

size_t choise(char c) {
  if (c) {
    return 4;
  } else {
    return 8;
  }
}

class Streebog {
public:

    Streebog() {
        IV.resize(64);
        N.resize(64);
        Sigma.resize(64);
        v0.resize(64);
        v512.resize(64);
    }
    void Hash(const int mode256, const vector<unsigned char>& message, ull length, vector<unsigned char>& res) {
        if (mode256) {
            for (unsigned char& a : IV) {
                a = 0x01;
            }
        }
        else {
            for (unsigned char& a : IV) {
                a = 0x00;
            }
        }
        for (int i = 0; i < 64; i++) {
            N[i] = Sigma[i] = v0[i] = v512[i] = 0;
        }
        v512[62] = 0x2;
        vector<unsigned char> hash = IV, m;
        m.resize(64);
        while (length >= 512) {
            int charshift = (length + 1) / 8 - 64;
            for (int i = 0; i < 64; i++) {
                m[i] = message[charshift + i];
            }
            g_N(N, hash, m);
            AddMod512(N, v512, N);
            AddMod512(Sigma, m, Sigma);
            length -= 512;
        }

        int charshift = 63 - ((length + 1) / 8 - 1);
        for (int i = 0; i < charshift + 1; i++) {
            m[i] = 0;
        }
        for (int i = 0; i < (length + 1) / 8; i++) {
            m[charshift + i] = message[i];
        }
        m[63 - length / 8] += 1 << (length % 8);

        g_N(N, hash, m);
        v512[63] = length & 0xFF;
        v512[62] = length >> 8;
        AddMod512(N, v512, N);

        AddMod512(Sigma, m, Sigma);

        g_N(v0, hash, N);
        g_N(v0, hash, Sigma);
        int up = (mode256) ? 32 : 64;
        for (int i = 0; i < up; i++) {
            res[i] = hash[i];
        }
    }

    int Test(int is_256, int is_512, int testStartNum) {
        vector<unsigned char> res512, res256;
        res512.resize(64);
        res256.resize(32);
        vector<pair<vector<unsigned char>, int>> testSuiteM = {
            {{
                0x32, 0x31, 0x30, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33,
                0x32, 0x31, 0x30, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33,
                0x32, 0x31, 0x30, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33,
                0x32, 0x31, 0x30, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33,
                0x32, 0x31, 0x30, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33,
                0x32, 0x31, 0x30, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33,
                0x32, 0x31, 0x30
            }, 504},
            {{
                0xfb, 0xe2, 0xe5, 0xf0, 0xee, 0xe3, 0xc8, 0x20, 0xfb, 0xea, 0xfa,
                0xeb, 0xef, 0x20, 0xff, 0xfb, 0xf0, 0xe1, 0xe0, 0xf0, 0xf5, 0x20,
                0xe0, 0xed, 0x20, 0xe8, 0xec, 0xe0, 0xeb, 0xe5, 0xf0, 0xf2, 0xf1,
                0x20, 0xff, 0xf0, 0xee, 0xec, 0x20, 0xf1, 0x20, 0xfa, 0xf2, 0xfe,
                0xe5, 0xe2, 0x20, 0x2c, 0xe8, 0xf6, 0xf3, 0xed, 0xe2, 0x20, 0xe8,
                0xe6, 0xee, 0xe1, 0xe8, 0xf0, 0xf2, 0xd1, 0x20, 0x2c, 0xe8, 0xf0,
                0xf2, 0xe5, 0xe2, 0x20, 0xe5, 0xd1
            }, 576 }
        };
        vector<vector<unsigned char>> testSuiteH512 = {
            {
                0x48, 0x6f, 0x64, 0xc1, 0x91, 0x78, 0x79, 0x41, 0x7f, 0xef, 0x08, 0x2b, 0x33, 0x81, 0xa4, 0xe2,
                0x11, 0xc3, 0x24, 0xf0, 0x74, 0x65, 0x4c, 0x38, 0x82, 0x3a, 0x7b, 0x76, 0xf8, 0x30, 0xad, 0x00,
                0xfa, 0x1f, 0xba, 0xe4, 0x2b, 0x12, 0x85, 0xc0, 0x35, 0x2f, 0x22, 0x75, 0x24, 0xbc, 0x9a, 0xb1,
                0x62, 0x54, 0x28, 0x8d, 0xd6, 0x86, 0x3d, 0xcc, 0xd5, 0xb9, 0xf5, 0x4a, 0x1a, 0xd0, 0x54, 0x1b
            },
            {
                0x28, 0xfb, 0xc9, 0xba, 0xda, 0x03, 0x3b, 0x14, 0x60, 0x64, 0x2b, 0xdc, 0xdd, 0xb9, 0x0c, 0x3f,
                0xb3, 0xe5, 0x6c, 0x49, 0x7c, 0xcd, 0x0f, 0x62, 0xb8, 0xa2, 0xad, 0x49, 0x35, 0xe8, 0x5f, 0x03,
                0x76, 0x13, 0x96, 0x6d, 0xe4, 0xee, 0x00, 0x53, 0x1a, 0xe6, 0x0f, 0x3b, 0x5a, 0x47, 0xf8, 0xda,
                0xe0, 0x69, 0x15, 0xd5, 0xf2, 0xf1, 0x94, 0x99, 0x6f, 0xca, 0xbf, 0x26, 0x22, 0xe6, 0x88, 0x1e
            }
        };
        vector<vector<unsigned char>> testSuiteH256 = {
            {
                0x00, 0x55, 0x7b, 0xe5, 0xe5, 0x84, 0xfd, 0x52, 0xa4, 0x49, 0xb1, 0x6b, 0x02, 0x51, 0xd0, 0x5d,
                0x27, 0xf9, 0x4a, 0xb7, 0x6c, 0xba, 0xa6, 0xda, 0x89, 0x0b, 0x59, 0xd8, 0xef, 0x1e, 0x15, 0x9d
            },
            {
                0x50, 0x8f, 0x7e, 0x55, 0x3c, 0x06, 0x50, 0x1d, 0x74, 0x9a, 0x66, 0xfc, 0x28, 0xc6, 0xca, 0xc0,
                0xb0, 0x05, 0x74, 0x6d, 0x97, 0x53, 0x7f, 0xa8, 0x5d, 0x9e, 0x40, 0x90, 0x4e, 0xfe, 0xd2, 0x9d
            }
        };
        if (is_512) {
            for (int i = testStartNum; i < testSuiteM.size(); i++) {
                Hash(0, testSuiteM[i].first, testSuiteM[i].second, res512);
                if (res512 != testSuiteH512[i]) {
                    std::cout << "ERROR: 512-bit, testnum: " << i << std::endl;
                }
            }
        }
        if (is_256) {
            for (int i = testStartNum; i < testSuiteM.size(); i++) {
                Hash(1, testSuiteM[i].first, testSuiteM[i].second, res256);
                if (res256 != testSuiteH256[i]) {
                	std::cout << "ERROR: 256-bit, testnum: " << i << std::endl;
                }
            }
        }
        return 0;
    }
private:
    void AddMod512(const vector<unsigned char>& a, const vector<unsigned char>& b, vector<unsigned char>& c) {
        int t = 0;

        for (int i = 63; i >= 0; i--) {
            t = a[i] + b[i] + (t >> 8);
            c[i] = t & 0xFF;
        }
    }

    void X(const vector<unsigned char>& a, const vector<unsigned char>& b, vector<unsigned char>& c) {
        for (int i = 0; i < 64; i++) {
            c[i] = a[i] ^ b[i];
        }
    }

    void S(vector<unsigned char>& curr) {
        for (int i = 0; i < 64; i++) {
            curr[i] = STable[curr[i]];
        }
    }

    void P(vector<unsigned char>& curr) {
        for (int i = 0; i < 8; i++) {
            for (int j = i + 1; j < 8; j++) {
                std::swap(curr[i * 8 + j], curr[Tau[i * 8 + j]]);
            }
        }
    }

    void L(vector<unsigned char>& curr) {
        for (int i = 0; i < 8; i++) {
            ull tmp = 0;
            for (int j = 0; j < 8; j++) {
                for (int k = 0; k < 8; k++) {
                    if ((curr[i * 8 + j] & (1 << (7 - k))) != 0) {
                        tmp ^= A[j * 8 + k];
                    }
                }
            }
            for (int j = 0; j < 8; j++) {
                curr[i * 8 + j] = (tmp & ((ull)0xFF << (7 - j) * 8)) >> (7 - j) * 8;
            }
        }
    }

    void KeyCalc(vector<unsigned char>& K, int i) {
        X(K, C[i], K);
        S(K);
        P(K);
        L(K);
    }

    void E(vector<unsigned char>& K, const vector<unsigned char>& m, vector<unsigned char>& curr) {
        X(K, m, curr);
        for (int i = 0; i < 12; i++) {
            S(curr);
            P(curr);
            L(curr);
            KeyCalc(K, i);
            X(curr, K, curr);
        }
    }

    void g_N(vector<unsigned char>& N, vector<unsigned char>& h, const vector<unsigned char>& m) {
        vector<unsigned char> t, K;
        t.resize(64);
        K.resize(64);

        X(N, h, K);

        S(K);
        P(K);
        L(K);

        E(K, m, t);

        X(t, h, t);
        X(t, m, h);
    }


    const vector<unsigned char> STable = {
        0xFC, 0xEE, 0xDD, 0x11, 0xCF, 0x6E, 0x31, 0x16, 0xFB, 0xC4, 0xFA, 0xDA, 0x23, 0xC5, 0x04, 0x4D,
        0xE9, 0x77, 0xF0, 0xDB, 0x93, 0x2E, 0x99, 0xBA, 0x17, 0x36, 0xF1, 0xBB, 0x14, 0xCD, 0x5F, 0xC1,
        0xF9, 0x18, 0x65, 0x5A, 0xE2, 0x5C, 0xEF, 0x21, 0x81, 0x1C, 0x3C, 0x42, 0x8B, 0x01, 0x8E, 0x4F,
        0x05, 0x84, 0x02, 0xAE, 0xE3, 0x6A, 0x8F, 0xA0, 0x06, 0x0B, 0xED, 0x98, 0x7F, 0xD4, 0xD3, 0x1F,
        0xEB, 0x34, 0x2C, 0x51, 0xEA, 0xC8, 0x48, 0xAB, 0xF2, 0x2A, 0x68, 0xA2, 0xFD, 0x3A, 0xCE, 0xCC,
        0xB5, 0x70, 0x0E, 0x56, 0x08, 0x0C, 0x76, 0x12, 0xBF, 0x72, 0x13, 0x47, 0x9C, 0xB7, 0x5D, 0x87,
        0x15, 0xA1, 0x96, 0x29, 0x10, 0x7B, 0x9A, 0xC7, 0xF3, 0x91, 0x78, 0x6F, 0x9D, 0x9E, 0xB2, 0xB1,
        0x32, 0x75, 0x19, 0x3D, 0xFF, 0x35, 0x8A, 0x7E, 0x6D, 0x54, 0xC6, 0x80, 0xC3, 0xBD, 0x0D, 0x57,
        0xDF, 0xF5, 0x24, 0xA9, 0x3E, 0xA8, 0x43, 0xC9, 0xD7, 0x79, 0xD6, 0xF6, 0x7C, 0x22, 0xB9, 0x03,
        0xE0, 0x0F, 0xEC, 0xDE, 0x7A, 0x94, 0xB0, 0xBC, 0xDC, 0xE8, 0x28, 0x50, 0x4E, 0x33, 0x0A, 0x4A,
        0xA7, 0x97, 0x60, 0x73, 0x1E, 0x00, 0x62, 0x44, 0x1A, 0xB8, 0x38, 0x82, 0x64, 0x9F, 0x26, 0x41,
        0xAD, 0x45, 0x46, 0x92, 0x27, 0x5E, 0x55, 0x2F, 0x8C, 0xA3, 0xA5, 0x7D, 0x69, 0xD5, 0x95, 0x3B,
        0x07, 0x58, 0xB3, 0x40, 0x86, 0xAC, 0x1D, 0xF7, 0x30, 0x37, 0x6B, 0xE4, 0x88, 0xD9, 0xE7, 0x89,
        0xE1, 0x1B, 0x83, 0x49, 0x4C, 0x3F, 0xF8, 0xFE, 0x8D, 0x53, 0xAA, 0x90, 0xCA, 0xD8, 0x85, 0x61,
        0x20, 0x71, 0x67, 0xA4, 0x2D, 0x2B, 0x09, 0x5B, 0xCB, 0x9B, 0x25, 0xD0, 0xBE, 0xE5, 0x6C, 0x52,
        0x59, 0xA6, 0x74, 0xD2, 0xE6, 0xF4, 0xB4, 0xC0, 0xD1, 0x66, 0xAF, 0xC2, 0x39, 0x4B, 0x63, 0xB6
    };

    const vector<unsigned char> Tau = {
         0,  8, 16, 24, 32, 40, 48, 56,
         1,  9, 17, 25, 33, 41, 49, 57,
         2, 10, 18, 26, 34, 42, 50, 58,
         3, 11, 19, 27, 35, 43, 51, 59,
         4, 12, 20, 28, 36, 44, 52, 60,
         5, 13, 21, 29, 37, 45, 53, 61,
         6, 14, 22, 30, 38, 46, 54, 62,
         7, 15, 23, 31, 39, 47, 55, 63
    };
    const vector<unsigned long long> A = {
        0x8e20faa72ba0b470, 0x47107ddd9b505a38, 0xad08b0e0c3282d1c, 0xd8045870ef14980e,
        0x6c022c38f90a4c07, 0x3601161cf205268d, 0x1b8e0b0e798c13c8, 0x83478b07b2468764,
        0xa011d380818e8f40, 0x5086e740ce47c920, 0x2843fd2067adea10, 0x14aff010bdd87508,
        0x0ad97808d06cb404, 0x05e23c0468365a02, 0x8c711e02341b2d01, 0x46b60f011a83988e,
        0x90dab52a387ae76f, 0x486dd4151c3dfdb9, 0x24b86a840e90f0d2, 0x125c354207487869,
        0x092e94218d243cba, 0x8a174a9ec8121e5d, 0x4585254f64090fa0, 0xaccc9ca9328a8950,
        0x9d4df05d5f661451, 0xc0a878a0a1330aa6, 0x60543c50de970553, 0x302a1e286fc58ca7,
        0x18150f14b9ec46dd, 0x0c84890ad27623e0, 0x0642ca05693b9f70, 0x0321658cba93c138,
        0x86275df09ce8aaa8, 0x439da0784e745554, 0xafc0503c273aa42a, 0xd960281e9d1d5215,
        0xe230140fc0802984, 0x71180a8960409a42, 0xb60c05ca30204d21, 0x5b068c651810a89e,
        0x456c34887a3805b9, 0xac361a443d1c8cd2, 0x561b0d22900e4669, 0x2b838811480723ba,
        0x9bcf4486248d9f5d, 0xc3e9224312c8c1a0, 0xeffa11af0964ee50, 0xf97d86d98a327728,
        0xe4fa2054a80b329c, 0x727d102a548b194e, 0x39b008152acb8227, 0x9258048415eb419d,
        0x492c024284fbaec0, 0xaa16012142f35760, 0x550b8e9e21f7a530, 0xa48b474f9ef5dc18,
        0x70a6a56e2440598e, 0x3853dc371220a247, 0x1ca76e95091051ad, 0x0edd37c48a08a6d8,
        0x07e095624504536c, 0x8d70c431ac02a736, 0xc83862965601dd1b, 0x641c314b2b8ee083
    };
    const vector<vector<unsigned char>> C = {
        {
            0xb1,0x08,0x5b,0xda,0x1e,0xca,0xda,0xe9,0xeb,0xcb,0x2f,0x81,0xc0,0x65,0x7c,0x1f,
            0x2f,0x6a,0x76,0x43,0x2e,0x45,0xd0,0x16,0x71,0x4e,0xb8,0x8d,0x75,0x85,0xc4,0xfc,
            0x4b,0x7c,0xe0,0x91,0x92,0x67,0x69,0x01,0xa2,0x42,0x2a,0x08,0xa4,0x60,0xd3,0x15,
            0x05,0x76,0x74,0x36,0xcc,0x74,0x4d,0x23,0xdd,0x80,0x65,0x59,0xf2,0xa6,0x45,0x07
        },
        {
            0x6f,0xa3,0xb5,0x8a,0xa9,0x9d,0x2f,0x1a,0x4f,0xe3,0x9d,0x46,0x0f,0x70,0xb5,0xd7,
            0xf3,0xfe,0xea,0x72,0x0a,0x23,0x2b,0x98,0x61,0xd5,0x5e,0x0f,0x16,0xb5,0x01,0x31,
            0x9a,0xb5,0x17,0x6b,0x12,0xd6,0x99,0x58,0x5c,0xb5,0x61,0xc2,0xdb,0x0a,0xa7,0xca,
            0x55,0xdd,0xa2,0x1b,0xd7,0xcb,0xcd,0x56,0xe6,0x79,0x04,0x70,0x21,0xb1,0x9b,0xb7
        },
        {
            0xf5,0x74,0xdc,0xac,0x2b,0xce,0x2f,0xc7,0x0a,0x39,0xfc,0x28,0x6a,0x3d,0x84,0x35,
            0x06,0xf1,0x5e,0x5f,0x52,0x9c,0x1f,0x8b,0xf2,0xea,0x75,0x14,0xb1,0x29,0x7b,0x7b,
            0xd3,0xe2,0x0f,0xe4,0x90,0x35,0x9e,0xb1,0xc1,0xc9,0x3a,0x37,0x60,0x62,0xdb,0x09,
            0xc2,0xb6,0xf4,0x43,0x86,0x7a,0xdb,0x31,0x99,0x1e,0x96,0xf5,0x0a,0xba,0x0a,0xb2
        },
        {
            0xef,0x1f,0xdf,0xb3,0xe8,0x15,0x66,0xd2,0xf9,0x48,0xe1,0xa0,0x5d,0x71,0xe4,0xdd,
            0x48,0x8e,0x85,0x7e,0x33,0x5c,0x3c,0x7d,0x9d,0x72,0x1c,0xad,0x68,0x5e,0x35,0x3f,
            0xa9,0xd7,0x2c,0x82,0xed,0x03,0xd6,0x75,0xd8,0xb7,0x13,0x33,0x93,0x52,0x03,0xbe,
            0x34,0x53,0xea,0xa1,0x93,0xe8,0x37,0xf1,0x22,0x0c,0xbe,0xbc,0x84,0xe3,0xd1,0x2e
        },
        {
            0x4b,0xea,0x6b,0xac,0xad,0x47,0x47,0x99,0x9a,0x3f,0x41,0x0c,0x6c,0xa9,0x23,0x63,
            0x7f,0x15,0x1c,0x1f,0x16,0x86,0x10,0x4a,0x35,0x9e,0x35,0xd7,0x80,0x0f,0xff,0xbd,
            0xbf,0xcd,0x17,0x47,0x25,0x3a,0xf5,0xa3,0xdf,0xff,0x00,0xb7,0x23,0x27,0x1a,0x16,
            0x7a,0x56,0xa2,0x7e,0xa9,0xea,0x63,0xf5,0x60,0x17,0x58,0xfd,0x7c,0x6c,0xfe,0x57
        },
        {
            0xae,0x4f,0xae,0xae,0x1d,0x3a,0xd3,0xd9,0x6f,0xa4,0xc3,0x3b,0x7a,0x30,0x39,0xc0,
            0x2d,0x66,0xc4,0xf9,0x51,0x42,0xa4,0x6c,0x18,0x7f,0x9a,0xb4,0x9a,0xf0,0x8e,0xc6,
            0xcf,0xfa,0xa6,0xb7,0x1c,0x9a,0xb7,0xb4,0x0a,0xf2,0x1f,0x66,0xc2,0xbe,0xc6,0xb6,
            0xbf,0x71,0xc5,0x72,0x36,0x90,0x4f,0x35,0xfa,0x68,0x40,0x7a,0x46,0x64,0x7d,0x6e
        },
        {
            0xf4,0xc7,0x0e,0x16,0xee,0xaa,0xc5,0xec,0x51,0xac,0x86,0xfe,0xbf,0x24,0x09,0x54,
            0x39,0x9e,0xc6,0xc7,0xe6,0xbf,0x87,0xc9,0xd3,0x47,0x3e,0x33,0x19,0x7a,0x93,0xc9,
            0x09,0x92,0xab,0xc5,0x2d,0x82,0x2c,0x37,0x06,0x47,0x69,0x83,0x28,0x4a,0x05,0x04,
            0x35,0x17,0x45,0x4c,0xa2,0x3c,0x4a,0xf3,0x88,0x86,0x56,0x4d,0x3a,0x14,0xd4,0x93
        },
        {
            0x9b,0x1f,0x5b,0x42,0x4d,0x93,0xc9,0xa7,0x03,0xe7,0xaa,0x02,0x0c,0x6e,0x41,0x41,
            0x4e,0xb7,0xf8,0x71,0x9c,0x36,0xde,0x1e,0x89,0xb4,0x44,0x3b,0x4d,0xdb,0xc4,0x9a,
            0xf4,0x89,0x2b,0xcb,0x92,0x9b,0x06,0x90,0x69,0xd1,0x8d,0x2b,0xd1,0xa5,0xc4,0x2f,
            0x36,0xac,0xc2,0x35,0x59,0x51,0xa8,0xd9,0xa4,0x7f,0x0d,0xd4,0xbf,0x02,0xe7,0x1e
        },
        {
            0x37,0x8f,0x5a,0x54,0x16,0x31,0x22,0x9b,0x94,0x4c,0x9a,0xd8,0xec,0x16,0x5f,0xde,
            0x3a,0x7d,0x3a,0x1b,0x25,0x89,0x42,0x24,0x3c,0xd9,0x55,0xb7,0xe0,0x0d,0x09,0x84,
            0x80,0x0a,0x44,0x0b,0xdb,0xb2,0xce,0xb1,0x7b,0x2b,0x8a,0x9a,0xa6,0x07,0x9c,0x54,
            0x0e,0x38,0xdc,0x92,0xcb,0x1f,0x2a,0x60,0x72,0x61,0x44,0x51,0x83,0x23,0x5a,0xdb
        },
        {
            0xab,0xbe,0xde,0xa6,0x80,0x05,0x6f,0x52,0x38,0x2a,0xe5,0x48,0xb2,0xe4,0xf3,0xf3,
            0x89,0x41,0xe7,0x1c,0xff,0x8a,0x78,0xdb,0x1f,0xff,0xe1,0x8a,0x1b,0x33,0x61,0x03,
            0x9f,0xe7,0x67,0x02,0xaf,0x69,0x33,0x4b,0x7a,0x1e,0x6c,0x30,0x3b,0x76,0x52,0xf4,
            0x36,0x98,0xfa,0xd1,0x15,0x3b,0xb6,0xc3,0x74,0xb4,0xc7,0xfb,0x98,0x45,0x9c,0xed
        },
        {
            0x7b,0xcd,0x9e,0xd0,0xef,0xc8,0x89,0xfb,0x30,0x02,0xc6,0xcd,0x63,0x5a,0xfe,0x94,
            0xd8,0xfa,0x6b,0xbb,0xeb,0xab,0x07,0x61,0x20,0x01,0x80,0x21,0x14,0x84,0x66,0x79,
            0x8a,0x1d,0x71,0xef,0xea,0x48,0xb9,0xca,0xef,0xba,0xcd,0x1d,0x7d,0x47,0x6e,0x98,
            0xde,0xa2,0x59,0x4a,0xc0,0x6f,0xd8,0x5d,0x6b,0xca,0xa4,0xcd,0x81,0xf3,0x2d,0x1b
        },
        {
            0x37,0x8e,0xe7,0x67,0xf1,0x16,0x31,0xba,0xd2,0x13,0x80,0xb0,0x04,0x49,0xb1,0x7a,
            0xcd,0xa4,0x3c,0x32,0xbc,0xdf,0x1d,0x77,0xf8,0x20,0x12,0xd4,0x30,0x21,0x9f,0x9b,
            0x5d,0x80,0xef,0x9d,0x18,0x91,0xcc,0x86,0xe7,0x1d,0xa4,0xaa,0x88,0xe1,0x28,0x52,
            0xfa,0xf4,0x17,0xd5,0xd9,0xb2,0x1b,0x99,0x48,0xbc,0x92,0x4a,0xf1,0x1b,0xd7,0x20
        }
    };
    vector<unsigned char> IV, N, Sigma, v0, v512;
};

ostream& operator<<(ostream& os, vector<unsigned char> message)
{
    for (int i = message.size(); i > 0; i--) {
        os << message[i];
    }
    return os;
}

void write_key_data(const char * filename, void * data, size_t length){
	std::ofstream key_file {filename, std::ios::binary};
	if (not key_file.is_open()) throw std::runtime_error(std::string("Error")+std::string(strerror(errno)));

	key_file.write(reinterpret_cast<const char*>(data), length);
	key_file.close();
}

void generate_random_key(const std::string& filename, int key_size){
	 std::random_device rd;
	  std::mt19937 gen(rd());
	  std::vector<uint8_t> key(key_size);
	  std::uniform_int_distribution<uint32_t> distrib(0, 0xff);

	  for (int i = 0; i < key_size; i++) {
	    key[i] = static_cast<uint8_t>(distrib(gen));
	  }
	  std::ofstream outfile(filename, std::ios::binary);
	  outfile.write(reinterpret_cast<const char*>(key.data()), key_size);
	  outfile.close();
}

void IVread(const std::vector <char> &c_fl, char *(&read_ptr), std::vector <unsigned long long> &IV) {
  unsigned char ch;
  std::ifstream in(read_ptr, std::ios::binary);
  unsigned int length = file_check(in);
  if ((c_fl[3] && (length != 4)) || ((length % 4) != 0))
    throw "Wrong CTR size!\n";
  size_t j = 0;
  size_t k = choise(c_fl[3]);
  while (j < length) {
    unsigned long long temp;
    for (size_t i = 0; i < k; i++) {
      in.read((char*)&(ch), sizeof(ch));
      temp = (temp << 8) + (unsigned int)ch;
      j++;
    }
    IV.push_back(temp);
  }
  if (k == 4) {
    IV[0] = (IV[0] << 32);
  }
  in.close();
}

void lsv(std::vector<unsigned char>& vec, unsigned int shiftAmount) {
    for (unsigned int i = 0; i < vec.size(); ++i) {
        vec[i] <<= shiftAmount;
    }
}

void rsv(std::vector<unsigned char>& vec, unsigned int shiftAmount) {
    for (unsigned int i = 0; i < vec.size(); ++i) {
        vec[i] >>= shiftAmount;
    }
}

void ecb(int choose,
            const std::vector <unsigned long long> &text,
            std::vector <unsigned long long> &ctext,
            std::vector <unsigned long long> &etext,
            const std::vector <unsigned int> &key) {
  unsigned int a0, a1;
  std::ofstream outfile("my.bin", std::ios::binary);

  if (choose == 1) {
    for (size_t i = 0; i < text.size(); i++) {
    	std::cout<<text.size()<<std::endl;

    	std::cout<<text[i]<<std::endl;

      a1 = text[i]   >> 32;
  	std::cout<<a1<<std::endl;

      a0 = (text[i] << 32) >> 32;
  	std::cout<<a0<<std::endl;

      for (size_t j = 1; j < 32; j++) {
        G(key[j], a1, a0);
      }
      ctext[i] = (( ((unsigned long long)g(key[32], a0)) ^ a1) << 32) + a0;
    }
    outfile.write(reinterpret_cast<const char*>(ctext.data()), ctext.size());
         	  outfile.close();
  } else if (choose == 2) {
    for (size_t i = 0; i < text.size(); i++) {
      ctext[i] = text[i];
      a1 = (ctext[i]) >> 32;
      a0 = ((ctext[i]) << 32) >> 32;
      etext[i] = dec(key, a1, a0);

    }
    outfile.write(reinterpret_cast<const char*>(etext.data()), etext.size());
             	  outfile.close();
  }
}





void ctr(const std::vector <char> &c_fl,
            const std::vector <unsigned long long> &text,
            std::vector <unsigned long long> &ctext,
            std::vector <unsigned long long> &etext,
            const std::vector <unsigned int> &key, char *argv[], int argc) {
  std::vector <unsigned long long> IV;
  unsigned int pos;
  if ((pos = search(argv, "-v", argc))) {
    IVread(c_fl, argv[pos], IV);
  } else {
    IV.push_back(0);
  }
  std::vector <unsigned long long> CTR;
  CTR.push_back(IV[0]);
  for (size_t i = 0; i < text.size()-1; i++) {
    CTR.push_back(CTR[i]+1);
  }
  if (c_fl[8]) {
    ctr_enc(c_fl, text, ctext, etext, key, CTR);
  } else if (c_fl[9]) {
    ctr_dec(c_fl, text, ctext, etext, key, CTR);
  } else {
    throw "CTR Error!\n";
  }
}

void ctr_enc(const std::vector <char> &c_fl,
            const std::vector <unsigned long long> &text,
            std::vector <unsigned long long> &ctext,
            std::vector <unsigned long long> &etext,
            const std::vector <unsigned int> &key,
            std::vector <unsigned long long> CTR) {
  for (size_t i = 0; i < text.size(); i++) {
    unsigned int a0, a1;
    a1 = CTR[i] >> 32;
    a0 = (CTR[i] << 32) >> 32;
    for (size_t j = 1; j < 32; j++) {
      G(key[j], a1, a0);
    }
    ctext[i] = (( ((unsigned long long)g(key[32], a0)) ^ a1) << 32) + a0;
    ctext[i] = text[i] ^ ((ctext[i] >> (64-s)) << (64-s));
  }
}

void ctr_dec(const std::vector <char> &c_fl,
            const std::vector <unsigned long long> &text,
            std::vector <unsigned long long> &ctext,
            std::vector <unsigned long long> &etext,
            const std::vector <unsigned int> &key,
            std::vector <unsigned long long> CTR) {
  for (size_t i = 0; i < text.size(); i++) {
    unsigned int a0, a1;
    a1 = CTR[i] >> 32;
    a0 = (CTR[i] << 32) >> 32;
    ctext[i] = text[i];
    for (size_t j = 1; j < 32; j++) {
      G(key[j], a1, a0);
    }
    etext[i] = (( ((unsigned long long)g(key[32], a0)) ^ a1) << 32) + a0;
    etext[i] = ctext[i] ^ ((etext[i] >> (64-s)) << (64-s));
  }
}

void cbc_enc(const std::vector <char> &c_fl,
            const std::vector <unsigned long long> &text,
            std::vector <unsigned long long> &ctext,
            std::vector <unsigned long long> &etext,
            const std::vector <unsigned int> &key,
            std::vector <unsigned long long> R) {
  for (size_t i = 0; i < text.size(); i++) {
    unsigned int a0, a1;
    a1 = (text[i] ^ R[i]) >> 32;
    a0 = ((text[i] ^ R[i]) << 32) >> 32;
    ctext[i] = enc(key, a1, a0);
    R.push_back(ctext[i]);
  }
}

void cbc_dec(const std::vector <char> &c_fl,
            const std::vector <unsigned long long> &text,
            std::vector <unsigned long long> &ctext,
            std::vector <unsigned long long> &etext,
            const std::vector <unsigned int> &key,
            std::vector <unsigned long long> R) {
  for (size_t i = 0; i < text.size(); i++) {
    unsigned int a0, a1;
    ctext[i] = text[i];
    a1 = (ctext[i]) >> 32;
    a0 = ((ctext[i]) << 32) >> 32;
    etext[i] = (dec(key, a1, a0)) ^ R[i];
    R.push_back(ctext[i]);
  }

}

void cbc(const std::vector <char> &c_fl,
            const std::vector <unsigned long long> &text,
            std::vector <unsigned long long> &ctext,
            std::vector <unsigned long long> &etext,
            const std::vector <unsigned int> &key, char *argv[], int argc) {
  std::vector <unsigned long long> IV;
  unsigned int pos;
  if ((pos = search(argv, "-v", argc))) {
    IVread(c_fl, argv[pos], IV);
  } else {
    IV.push_back(0);
  }
  std::vector <unsigned long long> R;
  for (size_t j = 0; j < IV.size(); j++) {
    R.push_back(IV[j]);
  }
  if (c_fl[8]) {
    cbc_enc(c_fl, text, ctext, etext, key, R);
  } else if (c_fl[9]) {
    cbc_dec(c_fl, text, ctext, etext, key, R);
  } else {
    throw "CBC Error!\n";
  }
}

std::vector<unsigned int> readKey( std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return {};
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::size_t numInts = static_cast<std::size_t>(fileSize) / sizeof(unsigned int);
    std::vector<unsigned int> key(numInts);

    file.read(reinterpret_cast<char*>(key.data()), fileSize);
    file.close();

    return key;
}
void printVector(const std::vector<unsigned long long>& data) {
    for (const auto& element : data) {
        std::cout << element << " ";
    }
    std::cout << std::endl;
}

std::vector<unsigned char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return {};
    }

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> data(fileSize);

    file.read(reinterpret_cast<char*>(data.data()), fileSize);

    file.close();
    return data;
}

std::vector<unsigned long long> convtoULL(const std::vector<unsigned char>& input) {
    const std::size_t numElements = input.size() / sizeof(unsigned long long);
    const unsigned char* byteData = input.data();
    const unsigned long long* longLongData = reinterpret_cast<const unsigned long long*>(byteData);

    std::vector<unsigned long long> output(longLongData, longLongData + numElements);

    return output;
}

void writeFile(const std::string& filename, const std::vector<unsigned long long>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to create file: " << filename << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(unsigned long long));
    file.close();
}

void test(const std::vector<unsigned long long>& vec) {
    for (const auto& value : vec) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
}
int main(int argc, char *argv[])
{
    		std::vector<unsigned char>	otext;
		    std::vector <unsigned long long> text;
		    std::vector <unsigned long long> ctext;
		    std::vector <unsigned long long> etext;
		    std::vector <unsigned char> init_text;
		    std::vector <unsigned char> ans_text;

		    std::vector <unsigned int> key;
	int size, mode;
	Streebog streebog;
//	const std::string& filename = "parole.key";
    vector<unsigned char> message;


    vector<unsigned char> res512, res256;


	int  k_size;
	std::string in_filename, out_filename, th_filename, key_string;
	int select_operation;
    ifstream file(th_filename, ifstream::ate | ifstream::in | ifstream::binary);

	std::cout<< "�������� ��������"<<std::endl;
	std::cout<< "1. C������� ��������� ����� � ������� ���� ����������� ����� ���-������� ���� 34.11"<<std::endl;
	std::cout<< "2. C������� ��������� ����� �� ������ ������"<<std::endl;
	std::cout<< "3. ������������ ������������� ��������� ����� � �������������� ���������� ��������� �����, � ���������� ���������� � ����� ����-��������� "<<std::endl;
	std::cout<< "4. ������������� ������������� ��������� ����� � �������������� ���������� ��������� �����, � ����������� ��� �� �����-����������"<<std::endl;
	std::cout<< "�����:"<<std::endl;

	std::cin>> select_operation;

if (select_operation == 1){
	  std::cout<< "������� ��� ��������� �����"<<std::endl;
	  std::cin >> th_filename;
	  std::cout<< "������� ������ �����"<<std::endl;
	  std::cin >> k_size;
	  generate_random_key(th_filename, k_size);
	  ifstream file(th_filename, ifstream::ate | ifstream::in | ifstream::binary);

	  size = file.tellg();
	  message.resize(size);
	  file.seekg(ios_base::beg);
	  file.read(reinterpret_cast<char*>(&message[0]), size);
	  file.close();
	  res512.resize(64);
	  res256.resize(32);

	  streebog.Hash(0, message, size, res512);
	  streebog.Hash(1, message, size, res256);
	//  std::string s(res512.begin(), res512.end());
//	  std::cout << "512: ";
	//  std::cout<<s;
//	  for (int i = 0; i < res512.size(); i++) {
//	  printf("%x", res512[i]);
	//  }
//	  std::cout << "\n256: ";
//	  for (int i = 0; i < res256.size(); i++) {
//	  printf("%x", res256[i]);
//	  }
	  std::ofstream outfile(th_filename, std::ios::binary);
	  outfile.write(reinterpret_cast<const char*>(res512.data()), res512.size());
	  outfile.close();
	  cout << endl;
}
else if (select_operation == 2){
	  std::cout<< "������� ������"<<std::endl;
	  std::cin>>key_string;
	  std::cout<< "������� ��� �����"<<std::endl;

		  std::cin >> th_filename;
		  std::ofstream outfile(th_filename, std::ios::binary);
		  outfile.write(reinterpret_cast<const char*>(key_string.c_str()), key_string.size());
		  outfile.close();
		  ifstream file(th_filename, ifstream::ate | ifstream::in | ifstream::binary);

		  size = file.tellg();
		  message.resize(size);
		  file.seekg(ios_base::beg);
		  file.read(reinterpret_cast<char*>(&message[0]), size);
		  file.close();
		  res512.resize(64);
		  res256.resize(32);
		  for (int i = 0; i < 10000; i++) {

		  streebog.Hash(0, message, size, res512);
		  streebog.Hash(1, message, size, res256);
		  }
		 // cout << "512: ";
		//  for (int i = 0; i < res512.size(); i++) {
		//  printf("%x", res512[i]);

		//  }
		//  cout << "\n256: ";
		//  for (int i = 0; i < res256.size(); i++) {
		//  printf("%x", res256[i]);
	//	  }
		  std::ofstream out(th_filename, std::ios::binary);
		 out.write(reinterpret_cast<const char*>(res512.data()), res512.size());

		  cout << endl;
}
else if (select_operation == 3){
	std::cout<<"������� �������� �����, ������� ������ �����������"<<std::endl;
	std::cin>>th_filename;
	otext = readFile(th_filename);
	text=convtoULL(otext);
//test(text);
	std::cout<<"������� �������� ����� �����"<<std::endl;
	std::cin>>key_string;
	std::vector<unsigned int> key = readKey(key_string);


	std::cout<<"������� �������� �������������� �����"<<std::endl;
	std::string cfile;
	std::cin>>cfile;



	std::cout << "�������� �����:"<< std::endl;
	std::cout << "1. ����� ������ (ECB)"<< std::endl;
	std::cout << "2. ����� ������������ (XOR)"<< std::endl;
	std::cout << "3. C�������� ������ �� ����������� (CBC)"<< std::endl;
	std::cin>>mode;

	switch (mode){

	case 1:
		ecb(1, text, ctext, etext, key);
	//	writeFile(cfile, ctext);
			break;
	case 2:
//		ctr(c_fl, text, ctext, etext, key, argv, argc);
			break;
	case 3:
//		cbc(c_fl, text, ctext, etext, key, argv, argc);
			break;

	default:
		std::cout << "Not found!"<< std::endl;

	}
}
else if (select_operation == 4){
	std::cout<<"������� �������� �����, ������� ������ �����������"<<std::endl;
		std::cin>>th_filename;
		otext = readFile(th_filename);
		//	text=convtoULL(otext);
		std::cout<<"������� �������� ����� �����"<<std::endl;
		std::cin>>key_string;
		std::vector<unsigned int> key = readKey(key_string);
		std::cout<<"������� �������� �������������� �����"<<std::endl;
		std::string efile;
		std::cin>>efile;
	std::cout << "�������� �����:"<< std::endl;
		std::cout << "1. ����� ������� ������ (ECB)"<< std::endl;
		std::cout << "2. ����� ������������ (CTR)"<< std::endl;
		std::cout << "3. C�������� ������ �� ����������� (CBC)"<< std::endl;
		std::cin>>mode;

		switch (mode){

		case 1:
			ecb(0, text, ctext, etext, key);
			//writeFile(efile, etext);

					break;
		case 2:
//				ctr(c_fl, text, ctext, etext, key, argv, argc);
					break;
		case 3:
//				cbc(c_fl, text, ctext, etext, key, argv, argc);
					break;

			default:
				std::cout << "Not found!"<< std::endl;

			}
}
else return 0;
}




