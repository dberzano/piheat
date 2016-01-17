#!/usr/bin/env python

from requests import get,post
from sys import exit,argv
from time import sleep
from random import uniform

def rnd(a, b, dec=0):
  return round(uniform(a, b), dec)

def main(argv):
  try:
    thingid = argv[1]
  except IndexError as e:
    print "Usage: %s thingid" % argv[0]
    exit(1)
  temp = 18
  temp_incr = 0.4
  while True:
    #temp = rnd(15, 25.4, 1)
    if temp > 22 or temp < 17:
      temp_incr = -temp_incr
    temp = temp + temp_incr
    humi = rnd(75, 85)
    print "%s: temp=%.1f humi=%.0f" % (thingid, temp, humi)
    post("https://dweet.io/dweet/for/%s" % thingid,
         data={ "temp": temp,
                "humi": humi })
    sleep(5)

if __name__ == "__main__":
  main(argv)
