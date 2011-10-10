#!/bin/sh
while true; do
  ./6switch aaaa::50:c4ff:fe04:8013 on 
  ./6switch aaaa::50:c4ff:fe04:800f on 
  ./6switch aaaa::50:c4ff:fe04:800a on 
  ./6switch aaaa::50:c4ff:fe04:8011 on 
  ./6switch aaaa::50:c4ff:fe04:8010 on 
  sleep 1 
  ./6switch aaaa::50:c4ff:fe04:8013 off
  ./6switch aaaa::50:c4ff:fe04:800f off
  ./6switch aaaa::50:c4ff:fe04:800a off
  ./6switch aaaa::50:c4ff:fe04:8011 off 
  ./6switch aaaa::50:c4ff:fe04:8010 off 
  sleep 1 
done
