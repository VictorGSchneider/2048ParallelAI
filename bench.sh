#!/usr/bin/env bash
# Mede o speedup: roda o GA com N threads (REPS vezes cada) e imprime a tabela.
# Uso: ./bench.sh [threads...]   (padrão: 1 2 4 8)   |   REPS=3 ./bench.sh
set -euo pipefail
cd "$(dirname "$0")"
make -s 2048-ga
THREADS=("${@:-1 2 4 8}")
read -ra THREADS <<< "${THREADS[*]}"
REPS=${REPS:-3}
CSV=speedup.csv
rm -f "$CSV"

for t in "${THREADS[@]}"; do
  for ((r = 0; r < REPS; r++)); do ./2048-ga "$t" "$CSV" > /dev/null; done
done

# média por nº de threads, speedup = T(1) / T(n), eficiência = speedup / n
printf "%8s %12s %10s %11s\n" threads tempo\(s\) speedup eficiência
awk -F, 'NR > 1 { s[$1] += $2; n[$1]++ }
  END {
    base = s[1] / n[1]
    for (t in s) { m = s[t] / n[t]; printf "%8d %12.3f %10.2f %10.0f%%\n", t, m, base / m, 100 * base / m / t }
  }' "$CSV" | sort -n
