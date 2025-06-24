#!/bin/bash

n=20 # Adjust this to control how many times the loop runs

for (( i=1 ; i<=n ; i++ ))
do
   R=$(($RANDOM % 900 + 100)) # Generate a random number between 100 and 999
   bot_name="te${R}stbot_${R}"
   user_name="user_te${R}stbot_${R}"
   
   (./bot_v1 localhost 6667 "" "$bot_name" "$user_name") &
done
wait