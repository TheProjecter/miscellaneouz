"""
Module: PYBOURSO
Author: Nicolas Falliere

Parses real-time stock quotes from boursorama.com.

You may use this module as a stock retriever API:

    import pybourso
    r = pybourso.get_stock('US9900751036') # NASDAQ Composite
    print(r['value'])

Demo script: see main() below.

Caution: The script might no longer work properly if Boursorama decides to change their HTML layout.

Gotchas:
- If an ISIN points to different quotes (eg, US2605661048 could be DOW Insustrials or DJ Industrial), resolve the conflict by using the raw code:
    e = get_stock('$INDU', raw=True)
The 'raw code' of a stock can be found by visiting the webpage, in this case:
    http://www.boursorama.com/cours.phtml?symbole=$INDU
- Use 'debug=True' to dump the HTML page in case you want to hunt down a parser bug.
- Customize the user-agent sent to the server with the 'useragent=X' optional paramter.

"""

import sys
import urllib2
import re
import time
import traceback


version = 0.1



def wget(url, useragent=''):

    if useragent:
        r = urllib2.Request(url, headers={'User-Agent': useragent})
        f = urllib2.urlopen(r)

    else:
        f = urllib2.urlopen(url)

    b = f.read()
    f.close()
    return b



def get_stock(code, raw=False, trycount=3, useragent='', debug=False):

    is_isin = False

    if raw:
        url = 'http://www.boursorama.com/cours.phtml?symbole=%s' % code

    else:
        code = code.upper()

        # ISIN or abbreviated code
        is_isin = re.match(r'[A-Z]{2}[A-Z0-9]{10}', code) != None

        # build the url
        if is_isin:
            url = 'http://www.boursorama.com/cours.phtml?symbole=%s' % code
        else:
            url = 'http://www.boursorama.com/cours.phtml?symbole=1rP%s' % code

    #url += '&vue=histo'
    #if when:
    #    (year, month, day) = when
    #    t = '%04d-%02d-%02d' % when
    #    url += '&date='+t

    # download the page
    while trycount:
        try:
            b = wget(url, useragent)
        except:
            trycount -= 1
            time.sleep(2)
        else:
            break
    if not trycount:
        return None

    # dump the downloaded Page
    if debug:
        f = open('lastpage.html', 'w')
        f.write(b)
        f.close()

    # get the stock name
    s = 'class="nomste"'
    m = re.search(r'<.+?>([ -~]+?)<', b[b.find(s) + len(s):])
    if not m:
        return None
    name = m.groups()[0]
    #print(name)

    # get the isin code
    if is_isin:
        isin = code
    else:
        s = 'class="isin"'
        m = re.search(r'([A-Z]{2}[A-Z0-9]{10})-', b[b.find(s) + len(s):])
        if not m:
            return None
        isin = m.groups()[0]
        #print(isin)

    # indice or real stock
    is_indice = b.find('Composition</a>') >= 0

    # isolate (roughly) the area likely to contain the quote's elements
    i0 = b.find('class="t03"')
    if i0 < 0:
        return None
    i1 = b.find('</ul>', i0)
    if i1 < 0:
        return None
    c = b[i0:i1].replace('\n', '').replace('\r', '')
    #print(c)

    # extract elements
    #m = re.match(r'.+?strong>(.+?)\(c\).+?([+-].+?)%.+?<li>.+?</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>', c)
    m = re.match(r'.+?strong>(.+?)</strong>.+?<li>.+?([+-].+?)%.+?</li>.+?<li>.+?</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>', c)
    if not m:
        return None
    r = m.groups()
    #print(r)

    # convert string elements to real number values
    results = []
    for i in range(len(r)):
        v = r[i].upper().replace(' ', '').replace('PTS', '').replace('(C)', '').replace('EUR', '').replace('USD', '').replace('M', '000000').replace('K', '000').replace('\x80', '')
        if v.find('.') >= 0:
            v = float(v)
        else:
            v = int(v)
        results.append(v)

    # associate 
    e = {
        'isin':         isin,
        'name':         name,
        'value':        results[0],
        'variation':    results[1],
        'volume':       results[2],
        'opening':      results[3],
        'highval':      results[4],
        'lowval':       results[5],
    }

    # done
    return e



#
# Demo
#
if __name__ == '__main__':

    if len(sys.argv) >= 2:
        codelist = sys.argv[1:]
    else:
        codelist = ['CAC', 'US38259P5089', 'FP', 'FR0000031122', 'JP3633400001',]
        #codelist = ['AF',]

    for code in codelist:

        print ('%s' % code)
        try:
            e = get_stock(code, useragent='Firefox/3.5.0', debug=False)
        except:
            e = None
            traceback.print_exc()
            
        if not e:
            print('  Error, could not get stock "%s"...' % code)
        else:
            print('  Name/ISIN: %s / %s' % (e['name'], e['isin']))
            print('  Value:     %.2f' % e['value'])
            print('  Variation: %.2f%%' % e['variation'])
            print('  Volume:    %d' % e['volume'])
            print('  T0,Lo,Hi:  %.2f, %.2f, %.2f' % (e['opening'], e['lowval'], e['highval']))
