#define PI 3.141592f

unsigned int xs;
static float psSinValues[4096];

unsigned long xor_seed(unsigned long seed);
int xor_shift(void);
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
