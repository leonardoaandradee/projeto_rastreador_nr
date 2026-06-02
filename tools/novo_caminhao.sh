#!/bin/bash

# Verifica se você digitou o nome do caminhão
if [ -z "$1" ]; then
  echo "⚠️ Erro: Você esqueceu de dizer o nome do caminhão!"
  echo "👉 Uso: bash tools/novo_caminhao.sh <nome-do-dispositivo>"
  echo "👉 Exemplo: bash tools/novo_caminhao.sh bau-002"
  exit 1
fi

DEVICE=$1
CERTS_DIR="emqx-broker/certs" # <-- CAMINHO ATUALIZADO AQUI!

echo "🚚 Registrando novo caminhão: $DEVICE..."
echo "========================================="

# 1. Cria a configuração na pasta de certificados
cat <<EOF > $CERTS_DIR/$DEVICE.cnf
[req]
distinguished_name=req_distinguished_name
req_extensions=req_ext
prompt=no

[req_distinguished_name]
CN=$DEVICE

[req_ext]
subjectAltName=@alt_names

[alt_names]
DNS.1=$DEVICE
EOF

# 2. Entra na pasta, forja o crachá e assina (escondendo os logs chatos do OpenSSL)
cd $CERTS_DIR
openssl genrsa -out $DEVICE.key 2048 2>/dev/null
openssl req -new -key $DEVICE.key -out $DEVICE.csr -config $DEVICE.cnf 2>/dev/null
openssl x509 -req -in $DEVICE.csr -CA cacert.pem -CAkey ca.key -CAcreateserial -out $DEVICE.crt -days 365 -sha256 -extensions req_ext -extfile $DEVICE.cnf 2>/dev/null
cd ../..

echo "✅ Crachá digital ($DEVICE.crt) forjado com sucesso!"
echo "⚙️ Injetando chaves no arquivo esp32-firmware/src/secrets.h..." # <-- TEXTO ATUALIZADO
echo "========================================="

# O símbolo '>' faz o terminal salvar o resultado no arquivo em vez de mostrar na tela!
python tools/gen_cert_code.py $DEVICE > esp32-firmware/src/secrets.h # <-- CAMINHO DO SECRETS.H ATUALIZADO!

echo "🚀 Tudo pronto! Pode compilar o código do $DEVICE."