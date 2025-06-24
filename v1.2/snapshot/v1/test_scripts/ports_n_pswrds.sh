#!/bin/sh

# This is an example of a script should we choose to utalize script testing.
# A simple example that can be used to practice.
# This script is used to test the ports and passwords of server start

output=$(./ft_irc 1234 "hell" 2>&1)
echo "captured output = $output"

if echo "$output" | grep "ERROR:"; then
	echo "ITS HERE GUYS WE HAVE IT"
fi
#port number provided is out of range (6660-6669)"