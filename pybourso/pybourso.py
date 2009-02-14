"""#--------------------------------------------------------------------------
==============================================================================

pybourso.py -- Nicolas Falliere, feb.09

Recupere les cours de bourse sur Boursorama en parsant la page HTML.
In fine, se comporte comme une API:

    import pybourso
    r = pybourso.get_stock('CAC')
    print r['cours']

Pour la liste des valeurs renvoyees par get_stock(), voir plus bas dans le
source.

Pour un apercu rapide, il y a un main() dans ce script.

Le script pourrait ne plus fonctionner s'ils decident de changer la
structure de la page Web. Dans ce cas, reportez le bug 

==============================================================================
#--------------------------------------------------------------------------"""


import sys
import urllib2
import re



def get_stock(code, debug=False):

    # uppercase
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
    #print url
    f = urllib2.urlopen(url)
    b = f.read()
    f.close()

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
    #print name

    # get the isin code
    if is_isin:
        isin = code;
    else:
        s = 'class="isin"'
        m = re.search(r'([A-Z]{2}[A-Z0-9]{10})-', b[b.find(s) + len(s):])
        if not m:
            return None
        isin = m.groups()[0]
        #print isin

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

    # extract elements
    m = re.match(r'.+?strong>(.+?)\(c\).+?([+-].+?)%.+?<li>.+?</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>.+?<li>(.+?)</li>', c)
    if not m:
        return None
    r = m.groups()
    #print r

    # convert string elements to real number values
    results = []
    for i in range(len(r)):
        v = r[i].upper().replace(' ', '').replace('M', '000000').replace('K', '000').replace('\x80', '')
        if v.find('.') >= 0:
            v = float(v)
        else:
            v = int(v)
        results.append(v)

    # associate 
    e = {
        'isin':         isin,

        # French names
        'nom':          name,
        'cours':        results[0],
        'variation':    results[1],
        'volume':       results[2],
        'ouverture':    results[3],
        'plushaut':     results[4],
        'plusbas':      results[5],

        # English names
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
# Test!
#
if __name__ == '__main__':

    codelist = ['CAC', 'FP', 'FR0000031122', 'US38259P5089']

    for code in codelist:
        print '%s' % code
        e = get_stock(code, debug=False)
        if not e:
            print '  Error, could not get stock "%s"...' % code
        else:
            print '  Name/ISIN: %s / %s' % (e['nom'], e['isin'])
            print '  Value:     %.2f' % e['cours']
            print '  Variation: %.2f' % e['variation']
            print e

