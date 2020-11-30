## DISCLAIMER: THIS PROJECT IS INCOMPLETE AS COMMITTED.  It does not perform the correlation from DR2->HIP completely.

# Gaia DR2 processor for interstitial use with Hipparcos for use with computer graphics.

The idea of this project is to provide a large subsection of the Gaia DR2 stars
(approximately 11M stars).  This is not particularly useful for astronomical
purposes, however, for general user interface applications, it provides a very
useful subset in a relatively easy-to-manage size.

DISCLAIMER: When I say this is not particularly useful, I am begging you not to
do science with the output of these tools.  I am making a *lot* of compromises
for the sake of performance, ease-of-use and visual appearance!

DISCLAIMER: This repository management for this repo is weird.  There will not
be history applied to this repo, and instead it will always be a truncated
repo.

The actual output of this tool limits the data for the stars to what would be
used to display them statically in the night sky.  I.e. mas (milli-arc-seconds,
1/mas = parseces), latitude, longitude, color (in *approximate* Hipparcos
colors).  NOTE: the hipparcos colors are used because there is much better
research and algorithms on that mapping space to RGB.


First, download the database.

```wget -l 2 -r http://cdn.gea.esac.esa.int/Gaia/gdr2/gaia_source/csv/```

Second, build 'process'

```make```

Third, process the stars.  This searches for stars that match the following
criteria `gmag < 13.5 && gmag > 0`. This weeds out a ton of useless celestial
cruft. 

```./process.sh```

The output from this first phase is `processed.csv` this basically includes
the lines from the original DR2 database as-is.  We can then use this to cross-
correlate with our Hipparcos dataset.

## The hip2.dat data parsing.

Included in this is a Hipparcos dataset parserser.  What I originally used to
generate a visually appealing starfield.  The issue is that it only contained
approximately 100,000 stars and a lot of the mas data was very, very bad.

This however does contain the HIP star numbers, which is VERY useful when cross
correlating to other databases and especially when correlating to the
Stellarium constellationship database.  

I have included `flat_stars.dat` in the `hipparcos` folder along with the files
used to generate that data.  The `flat_stars.dat` file contains star data in
the following format:

```
	RECSTR( flatstartype, typedef struct __attribute__((__packed__)) { \
		uint32_t rascention_bams; \
		int32_t  declination_bams; \
		int16_t parallax_10uas; \
		uint16_t magnitude_mag1000; \
		int16_t bvcolor_mag1000; \
		int16_t vicolor_mag1000; \
		uint32_t hipparcos_id; \
	} flat_star; );
```

And uses the following algorithm to convert for use in that file:

```
s.rascention_bams =  ( ( tmp = atof( fields[4] ) )/6.28318530718)*4294967295;
s.declination_bams = ( ( tmp = atof( fields[5] ) )/3.14159265359)*2147483647;
s.parallax_10uas = ( tmp = atof( fields[6] ) ) * 100;
s.magnitude_mag1000 = ( tmp = atof( fields[19] ) ) * 1000;
s.bvcolor_mag1000 = ( tmp = atof( fields[23] ) ) * 1000;
s.vicolor_mag1000 = ( tmp = atof( fields[25] ) ) * 1000;
s.hipparcos_id = hc; // atoi( fields[0] );
```

This is particularly useful for generally displaying stars.  And I have had
very good luck with it, so I intend to match this sort of mentality for the
rest of this project.

Now that we have `flat_stars.dat` we can begin the cross-correlation process.







