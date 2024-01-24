# refer to: 
#   - https://www.baeldung.com/openssl-self-signed-cert
#   - https://gist.github.com/Barakat/675c041fd94435b270a25b5881987a30
#   - https://stackoverflow.com/questions/18233835/creating-an-x509-v3-user-certificate-by-signing-csr
#   - https://certificatetools.com/
# cert settings
is_v1=$1

key_bits=2048
server_host="127.0.0.1"
hash_alg=-sha384
valid_years=1000

# compute valid_days
time_ts=`date +%s`
date_year=`date +%Y`
date_ts=`date -d @$time_ts --iso-8601=seconds`
date_expire_year=`expr $date_year + $valid_years`
date_expire_ts=${date_ts/$date_year/$date_expire_year}
time_expire_ts=`date -d $date_expire_ts +%s`
valid_days=$((($time_expire_ts - $time_ts) / 86400))
v3ext_file=`pwd`/v3.ext
org_name=yasio

issuer_cn="yasio.github.io"
issuer_subj="/C=CN/O=$org_name/CN=$issuer_cn"

# Create Self-Signed Root CA(Certificate Authority)
if [ ! -f "ca-prk.pem" ] ; then
  if [ "$is_v1" != 'true' ] ; then
    openssl req -newkey rsa:$key_bits $hash_alg -nodes -keyout ca-prk.pem -x509 -days $valid_days -out ca-cer.pem -subj "$issuer_subj"; cp ca-cer.pem ca-cer.crt
  else
    openssl genrsa -out ca-prk.pem $key_bits
    openssl req -new $hash_alg -key ca-prk.pem -out ca-csr.pem -subj "$issuer_subj"
    openssl x509 -req -signkey ca-prk.pem -in ca-csr.pem -out ca-cer.pem -days $valid_days
  fi
fi

# Server

# 1. Generate unencrypted 2048-bits RSA private key for the server (CA) & Generate CSR for the server

openssl req -newkey rsa:$key_bits $hash_alg -nodes -keyout server-prk.pem -out server-csr.pem -subj "/C=CN/O=$org_name/CN=$server_host"

# 2. Sign with our RootCA
if [ "$is_v1" != 'true' ] ; then
  # subjectAltName in extfile is important for browser visit
  openssl x509 -req $hash_alg -in server-csr.pem -CA ca-cer.pem -CAkey ca-prk.pem -CAcreateserial -out server-cer.pem -days $valid_days -extfile $v3ext_file; cp server-cer.pem server-cer.crt
else
  openssl x509 -req $hash_alg -in server-csr.pem -CA ca-cer.pem -CAkey ca-prk.pem -CAcreateserial -out server-cer.pem -days $valid_days; cp server-cer.pem server-cer.crt
fi

# Check if the certificate is signed properly
openssl x509 -in server-cer.pem -noout -text
