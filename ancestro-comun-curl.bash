#!/bin/bash

define(){ IFS='\n' read -r -d '' ${1} || true; }

# Esta consulta busca el primer ancestro común
# de 9 y 6 en el árbol "tree-01"
define DATA <<'EOF'
{
  "id":"tree-01",
  "node_a":3,
  "node_b":9
}
EOF

curl --header "Content-Type: application/json" \
     -G\
     -w'\n'\
     --data-urlencode "q=${DATA}" \
     http://localhost/ancestro-comun
