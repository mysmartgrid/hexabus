#!/bin/sh
export BASEURL=http://10.23.1.253
export URI=api/sensor/foo
export SERVICEURL=${BASEURL}:1234/${URI}
export DATAFILE=benchmark.log
echo "Starting benchmark on url ${SERVICEURL}"

for worker in 3000 3001 3002 3003; do
  worker_url=${BASEURL}:${worker}/${URI}
  echo "Initializing ${worker_url}"
  curl -i -H "Content-Type: application/json" -X PUT \
    -T create-sensor-payload.txt ${worker_url} 
  if [ $? -ne 0 ]; then
    echo "Cannot create sensor, aborting."
    exit
  fi
  echo "Submitting one test value."
  curl -i -H "Content-Type: application/json" -X POST \
    -T submit-value-payload.txt ${worker_url}
  if [ $? -ne 0 ]; then
    echo "Failed to submit sensor value, aborting."
    exit
  fi
done

echo "#### We're set. Proceeding to performance test."
ab -p submit-value-payload.txt -T application/json \
  -c 50 -n 25000 -e ${DATAFILE} \
  ${SERVICEURL} 
