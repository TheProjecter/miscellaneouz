#
# Compact Sudoku solver (requires Python 2.5)
# Nicolas Falliere
#


def dump(a):
    for i in xrange(81):
        if len(a[i]) == 1:
            print '%d' % a[i][0],
        elif len(a[i]) == 0:
            print 'x',
        else:
            print '-',
        if (i+1) % 9 == 0:
            print
    print


def simplify(a):
    mod = 0
    for i in xrange(0, 81, 9): # lines
        for j in xrange(i, i+9):
            if len(a[j]) == 1:
                for k in xrange(i, i+9):
                    if j != k and a[j][0] in a[k]:
                        a[k].remove(a[j][0])
                        mod += 1
    for i in xrange(0, 9): # columns
        for j in xrange(i, i+81, 9):
            if len(a[j]) == 1:
                for k in xrange(i, i+81, 9):
                    if j != k and a[j][0] in a[k]:
                        a[k].remove(a[j][0])
                        mod += 1
    for i in (0,3,6,27,30,33,54,57,60): # grids
        for j in (i,i+1,i+2,i+9,i+10,i+11,i+18,i+19,i+20):
            if len(a[j]) == 1:
                for k in (i,i+1,i+2,i+9,i+10,i+11,i+18,i+19,i+20):
                    if j != k and a[j][0] in a[k]:
                        a[k].remove(a[j][0])
                        mod += 1
    return mod


import copy
def solve(a):
    while simplify(a): pass
    (l, j) = (10, -1)
    for i in xrange(81):
        if len(a[i]) == 0:
            return []
        if len(a[i]) >= 2 and len(a[i]) < l:
            (l, j) = (len(a[i]), i)
    if j < 0:
        return a
    for i in xrange(len(a[j])):
        b = copy.deepcopy(a)
        b[j] = a[j][i:i+1]
        r = solve(b)
        if r:
            return r
    return []


if __name__ == '__main__':
    boards = (
        { 'board':      '     5    1 62 58  3 4 8   6 8   23  4     9  93   6 4   8 2 1  54 96 7    1     ',
          'solution':   '826935147419627583735418926678549231541263798293781654367852419154396872982174365' },
        { 'board':      '2   849 35   6 1   6  278  38   27 1    9    1 46   58  573  1   9 4   64 791   2',
          'solution':   '271584963548369127963127845386452791752891634194673258625738419819245376437916582' },
        { 'board':      '  9 7 61    4 87 3 38    9 97456   2 1     6 2   87349 5    82 1 72 5    92 1 4  ',
          'solution':   '549372618621498753738651294974563182813924567265187349456739821187245936392816475' },
        { 'board':      '53  7    6  195    98    6 8   6   34  8 3  17   2   6 6    28    419  5    8  79',
          'solution':   '534678912672195348198342567859761423426853791713924856961537284287419635345286179' },
        { 'board':      '  1234       5  1 567      3  8 7   12     96   9 1  5      482 9  8       5723  ',
          'solution':   '981234657243756918567198243359867124128345796476921835735619482692483571814572369' },
        { 'board':      '  73 61  8   4   7  6   9  3  9 1  6  1 8 7  5  2 4  1  5   4  2   1   3  94 72  ',
          'solution':   '427396185893145627156728934342971856961583742578264391715832469284619573639457218' },
        { 'board':      ' 1   234    456       3 678  62 9   8 7   1 9   8 54  435 9       627    625   9 ',
          'solution':   '619782345378456912254931678546219783827364159193875426435198267981627534762543891' },
        { 'board':      '1  2    345  6       758    1  4  9  6     5  3  1  7    926       3  287    5  1',
          'solution':   '178294563459361287623758419817542396964873152235619874381926745546137928792485631' },
    )
    for s in boards:
        a = map(lambda x: [int(x), ] if x != ' ' else range(1,10), s['board'])
        b = solve(a)
        dump(b)
        c = ''.join(map(lambda x: '%s' % x[0], b))
        if c != s['solution']:
            print 'Error!'
