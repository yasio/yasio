# refer to: https://gist.github.com/Barakat/675c041fd94435b270a25b5881987a30

# cert settings
key_bits=2048
server_host="127.0.0.1"
hash_alg=-sha256
valid_years=100

# compute valid_days
time_ts=`date +%s`
date_year=`date +%Y`
date_ts=`date -d @$time_ts --iso-8601=seconds`
date_expire_year=`expr $date_year + $valid_years`
date_expire_ts=${date_ts/$date_year/$date_expire_year}
time_expire_ts=`date -d $date_expire_ts +%s`
valid_days=$((($time_expire_ts - $time_ts) / 86400))

# Certificate Authority

# 1. Generate unencrypted 2048-bits RSA private key for the certificate authority (CA)

openssl genrsa -out ca-prk.pem $key_bits

# 2. Generate certificate signing request (CSR) for the CA

openssl req -new $hash_alg -key ca-prk.pem -out ca-csr.pem -subj "/C=CN/O=Simdsoft CA"

# 3. Self-sign the CSR and to generate a certificate for the CA

openssl x509 -req -signkey ca-prk.pem -in ca-csr.pem -out ca-cer.pem -days $valid_days

# 4. Add the CA certificate to the client trust chain. Now every certificate signed by this certificate is trusted by the client

# Server

# 1. Generate unencrypted 2048-bits RSA private key for the server (CA)

openssl genrsa -out server-prk.pem $key_bits

# 2. Generate CSR for the server

openssl req -new $hash_alg -key server-prk.pem -out server-csr.pem -subj "/C=CN/CN=$server_host"

# 3. Hand over the CSR to the CA for signing

# Certificate Authority

# 1. View the server CSR and verify its content:

openssl req -in server-csr.pem -noout -text

# 2. Sign the server CSR

openssl x509 -req $hash_alg -in server-csr.pem -CA ca-cer.pem -CAkey ca-prk.pem -CAcreateserial -out server-cer.pem -days $valid_days

# 3. Hand over the signed server certificate to the requester

# Server

# 1. Check if the certificate is signed properly
openssl x509 -in server-cer.pem -noout -text

# 2. Now the server certificate is ready for use
