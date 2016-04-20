#!/usr/bin/env python
# coding: utf-8

from plotly import plotly
from plotly.graph_objs import Scatter,Layout,Data,Figure
from requests import get
from json import dumps
from datetime import datetime,timedelta

def nowoffset(**offset):
  dt = datetime.utcnow() + timedelta(**offset)
  return dt.isoformat("T")+"Z"

print "https://data.sparkfun.com/output/KJ7xggbMYgF0AWp0pn9D.json?gte[timestamp]=%s" % nowoffset(hours=-12)
exit(43)

r = get("https://data.sparkfun.com/output/KJ7xggbMYgF0AWp0pn9D.json?gte[timestamp]=%s" % nowoffset(hours=-12))
r.raise_for_status()

#print dumps(r.json(), indent=2)

temp = [ float(x["temp"]) for x in r.json() ]
humi = [ float(x["humi"]) for x in r.json() ]
time = [ datetime.strptime(x["timestamp"], '%Y-%m-%dT%H:%M:%S.%fZ') for x in r.json() ]

plotly.sign_in("l3g3nd4ryf0x", "d7j0vdhey8")
plotly.plot(
  Figure(data=[ Scatter(x=time, y=temp, name="Temperature"),
                Scatter(x=time, y=humi, yaxis="y2", name="Humidity") ],
         layout=Layout(title="Temperature and humidity",
                       yaxis=dict(title="Temp [°C]",
                                  range=[10, 35]),
                       yaxis2=dict(title="Humidity [%]",
                                   side="right",
                                   showgrid=False,
                                   range=[0, 100],
                                   overlaying="y"))),
  filename="temp-humi",
  auto_open=False
)

# plotly.offline.plot({
#   "data": [ Scatter(x=time, y=temp, name="Temperature"),
#             Scatter(x=time, y=humi, yaxis="y2", name="Humidity") ],
#   "layout": Layout(title="Temperature and humidity",
#                    yaxis=dict(title="Temp [°C]",
#                               range=[10, 35]),
#                    yaxis2=dict(title="Humidity [%]",
#                                side="right",
#                                showgrid=False,
#                                range=[0, 100],
#                                overlaying="y"))
# })
