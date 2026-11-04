// coded by Danoni631 for some functions and variables

typedef unsigned char  BYTE, TBYTE, UCHAR; // 8 bits, byte, unsigned char
typedef unsigned short WORD, DBYTE, UWORD, USHORT; // 16 bits, word, double byte, unsigned word, unsigned short;
typedef unsigned int   DWORD, QBYTE, UINT; // 32 bits, double word, quad byte, unsigned int
typedef unsigned long  ODWORD, OQBYTE, ULONG; // 32 bits, other double word, other quad byte, unsigned long
typedef unsigned long long QWORD, OBYTE, ULLONG // 64 bits, quad word, octa byte, unsigned long long

typedef BYTE* LPBYTE;
typedef WORD* LPWORD;
typedef DWORD* LPDWORD;
typedef ODWORD* OLPDWORD;
typedef QWORD* LPQWORD;
typedef void* LPVOID;

#define NULL ((LPVOID)0)

typedef const char* STRING;
