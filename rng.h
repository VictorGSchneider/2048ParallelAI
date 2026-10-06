#ifndef RNG_H
#define RNG_H

// Mistura (splitmix32-like) para derivar seeds independentes a partir de (base, índice).
// Seeds consecutivas (base+i) geram sequências correlacionadas no rand_r; o hash evita isso.
static inline unsigned int mix_seed(unsigned int base, unsigned int idx) {
    unsigned int z = base + 0x9e3779b9u * (idx + 1u);
    z = (z ^ (z >> 16)) * 0x85ebca6bu;
    z = (z ^ (z >> 13)) * 0xc2b2ae35u;
    return z ^ (z >> 16);
}

#endif
