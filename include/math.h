typedef struct
{
    float x;
    float y;
    float z;
} Vertex;

#define PI 3.141592f
#define RANDMAX 32767
#define SUMLIM 50

#define abs(a) (((a) > 0) ? a : -(a))

unsigned int xs;
static float psSinValues[4096];

unsigned long xor_seed(unsigned long seed);
inline void reflect_2D(int* x, int* y, int w, int h);
int xor_shift(void);
int dword_xor_shift(void);
double sine(double x);
double cosine(double x);
double square_root(double x);
double exp2(double x);
float float_abs(double x);
int sign(int x);
int floor(double x);
double inverted_tangent(double x);
double inverted_tangent2(double y, double x);
double pow(double base, int exp);
