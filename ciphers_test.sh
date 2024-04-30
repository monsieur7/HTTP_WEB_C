#!/bin/bash

SERVER="127.0.0.1"
PORT="8080"
TIMEOUT="5"  # Timeout in seconds for each connection attempt

# Retrieve the list of all ciphers supported by OpenSSL
ALL_CIPHERS=$(openssl ciphers 'ALL:eNULL' | tr ':' '\n')

# Loop through each cipher and attempt a connection
for CIPHER in $ALL_CIPHERS; do
    echo "Testing cipher: $CIPHER"
    result=$(echo -n | openssl s_client -cipher "$CIPHER" -connect "${SERVER}:${PORT}" -servername "${SERVER}" -state -nbio 2>&1)
    # Check if the connection was successful and the cipher is supported
    if echo "$result" | grep "Cipher is "; then
        echo "Cipher supported: $CIPHER"
        # Add your logic here for further handling if needed
    else
        echo "Cipher not supported or connection failed: $CIPHER"
    fi

    sleep 1  # Optional: Add a delay between connection attempts to avoid overwhelming the server
done
