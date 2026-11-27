"""Finds each of the game's maps on a cluebook's pages (correlation of its walls with the page's lines)."""
import sys, numpy as np
sys.path.insert(0,'.')
from cluemap import render, ink, runs_mask
from match import geo_blocks
from numpy.fft import rfft2, irfft2

def wall_sides(bk):
    """[(x,y,horiz)] of the map's walls - horiz: the top side of square (x,y); else its left side"""
    H=set(); V=set()
    for y in range(16):
        for x in range(16):
            q=bk['sq'][y*16+x]
            if q[0]: H.add((x,y))
            if q[6]: H.add((x,y+1))
            if q[9]: V.add((x,y))
            if q[3]: V.add((x+1,y))
    return H,V

def template(H,V,c,shape):
    t=np.zeros(shape); s=np.zeros(shape)
    L=int(round(c))
    for x in range(17):
        for y in range(17):
            px=int(round(x*c)); py=int(round(y*c))
            if x<16:
                seg=(slice(py-1,py+2),slice(px+int(0.2*c),px+int(0.8*c)))
                if (x,y) in H: t[seg]=1
                else: s[seg]=1
            if y<16:
                seg=(slice(py+int(0.2*c),py+int(0.8*c)),slice(px-1,px+2))
                if (x,y) in V: t[seg]=1
                else: s[seg]=1
    return t,s

def page_lines(im):
    m=ink(im); L=12
    lines=(runs_mask(m,L,1)|runs_mask(m,L,0)).astype(float)
    # a little thicker: a line drawn a pixel or two off still meets the template
    from scipy.ndimage import binary_dilation
    return binary_dilation(lines,iterations=2).astype(float)

def correlate(page, tmpl):
    P=rfft2(page); T=rfft2(tmpl, s=page.shape)
    return irfft2(P*np.conj(T), s=page.shape)
