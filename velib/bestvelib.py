#-----------------------------------------------------------------------------
#
# Trouve les stations Velib bonus (V+) les plus proches de stations
# Velib classiques afin de collecter un maximum de bonus sans (trop)
# se fatiguer.
#
# Actuellement (Fervrier 2009), le meilleur choix est clairement de se
# rendre a la station 09018 (Place Pigalle) et de ramener le velib a la
# station V+ 09019 (Victor Masse).
#
# Nicolas Falliere, 2008-2009
#
#-----------------------------------------------------------------------------


import sys
import urllib2
import re
import math


if len(sys.argv) >= 2:
    f = open(sys.argv[1], 'r')
else:
    f = urllib2.urlopen('http://www.velib.paris.fr/service/carto')

buff = f.read().replace('\n', ' ')
f.close()

p = re.compile(r'name=\"(.+?)\".*?number=\"(\d+?)\".+?lat=\"(\S+?)\".+?lng=\"(\S+?)\".+?open=\"(\d)\".+?bonus=\"(\d)\"')
all = p.findall(buff)

stations = {}
stations_normal = []
stations_bonus = []

for r in all:

    (name, number, lat, lng, opened, bonus) = (r[0], int(r[1]), float(r[2]), float(r[3]), int(r[4]), int(r[5]))

    if opened == 0:
        continue

    stations[number] = name

    e = (number, lat, lng)
    if not bonus:
        stations_normal.append(e)
    else:
        stations_bonus.append(e)


couples = []

for (base, lat0, lng0) in stations_bonus:

    mindist = 100000000.0
    closest = 0

    for (current, lat, lng) in stations_normal:

        x = abs(lat - lat0)
        y = abs(lng - lng0)

        if x < mindist and y < mindist:

            dist = math.sqrt(x**2 + y**2)

            if dist < mindist:

                mindist = dist
                closest = current

    couples.append((base, closest, mindist))


# Print out best couples (V+ -> V)
couples.sort(lambda x,y: cmp(x[2], y[2]))
print 'Top 10 couples (Velib -> Velib+):'
for (base, closest, mindist) in couples[:10]:
    print '%s  ->  %s  (%f)' % (stations[base].ljust(40), stations[closest].ljust(40), mindist)
