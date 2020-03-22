#include <stdio.h>
#include "dr2.h"
#include "hipparcos/flat_stars.h"

#define CNRBTREE_IMPLEMENTATION
#include <cnrbtree.h>

flat_star * pHIPSource;
int hipstars;

struct flat_star_linked_list
{
	flat_star * data;
	struct flat_star_linked_list * next;
};

//#define intcomp(x, y) ( x - y )
#define IABS(x) (((x)<0)?(-(x)):(x))

typedef struct flat_star * fsptr;
typedef struct flat_star_linked_list * fsllptr; 
CNRBTREETEMPLATE( uint32_t, fsllptr, RBptrcmp, RBptrcpy, RBnullop );
CNRBTREETEMPLATE( uint32_t, fsptr, RBptrcmp, RBptrcpy, RBnullop );

flat_star * out_stars;
int num_out_stars = 0;

//This method is bad.  Go the other way around.
flat_star *  FindDRStar( uint32_t rascention_bams, int32_t declination_bams, int parallax, int mag, int on_behalf_of_id, int wrap, int *  bestscore );

cnrbtree_uint32_tfsllptr * hipset;
cnrbtree_uint32_tfsptr * hipsetbyhipid;
cnrbtree_uint32_tfsllptr * drset;

int main()
{
	hipset = cnrbtree_uint32_tfsllptr_create();
	hipsetbyhipid = cnrbtree_uint32_tfsptr_create();
	drset = cnrbtree_uint32_tfsllptr_create();
	out_stars = malloc( sizeof( flat_star ) );
	num_out_stars = 0;


	int i;
	{
		FILE * fHIP = fopen( "hipparcos/flat_stars.dat", "rb" );
		if( !fHIP || ferror( fHIP ) )
		{
			fprintf( stderr, "Error: cannot open hipparcos flat_stars.dat\n" );
			return -6;
		}
		fseek( fHIP, 0, SEEK_END );
		int hiplen = ftell( fHIP );
		fseek( fHIP, 0, SEEK_SET );
		pHIPSource = malloc( hiplen );
		hipstars = hiplen / sizeof( flat_star );
		fread( pHIPSource, hiplen, 1, fHIP );
		fclose( fHIP );
	}

	for( i = 0; i < hipstars; i++ )
	{
		flat_star * fsp = pHIPSource + i;
		uint32_t rabams = fsp->rascention_bams;
		struct flat_star_linked_list * lnew = malloc( sizeof( struct flat_star_linked_list ) );
		struct flat_star_linked_list * lold = RBHAS( hipset, rabams ) ? RBA( hipset, rabams ) : 0;
		lnew->next = lold;
		if( lold )	printf( "%p\n", lold );
		lnew->data = fsp;
		RBA( hipset, rabams ) = lnew;
		RBA( hipsetbyhipid, fsp->hipparcos_id ) = fsp;
	}

	//RBFOREACH( uint32_tfsllptr, hipset, p )
	//{
	//	printf( ".key = %d .myvalue = %d\n", p->key, p->data->data->rascention_bams );
	//}


	//Now that we've loaded the hipparcos data, we need to start processing the Gaia DR2 data.
	FILE * f = fopen( "processed.csv", "rb" );
	if( !f || ferror( f ) )
	{
		fprintf( stderr, "Error: could not open processed.csv\n" );
		exit( -9 );
	}

	//Perf check:
	// Using 'SplitStrings' : 1m45.426s
	// Being smart about making the line data : 0m31.107s
	// Being mega smart, and freading 8192 bytes: 0m17.331s (as fast as I think it can read off disk)
	int lineno = 0;
	char bufin[8192];
	int bufinpl = 0;
	int reads = 0;
	int field = 0;
	int linepl = 0;
	while( !feof( f ) )
	{
		char line[100][100];
		char c;
		int forcebreak = 0;
		do
		{
			if( bufinpl == reads )
			{
				reads = fread( bufin, 1, 8192, f );
				bufinpl = 0;
			}
			for( ; bufinpl < reads; )
			{
				c = bufin[bufinpl++]; // ) && bufinpl != reads && c != '\n' )
				if( c == '\n' ) { forcebreak = 1; break; }
				else if( c == ',' )
				{
					line[field++][linepl] = 0;
					linepl = 0;				
				}
				else
				{
					line[field][linepl++] = c;
				}
			}
		} while( !forcebreak );
		lineno++;
		line[field++][linepl] = 0;
		//printf( "L %d %d %d %d\n", field, linepl, bufinpl, reads );

		if( field != 94 )
		{
			printf( "Error on processed.csv line %d;  %d\n", lineno, field );
			field = 0;
			linepl = 0;
			continue;
		}
		if( lineno % 100000 == 0 ) printf( "Line: %d\n", lineno );

		do
		{
			double ra = atof( line[dr2_ra] );
			double dec = atof( line[dr2_dec] );
			double parallax = atof( line[dr2_parallax] );
			double parallax_over_error = atof( line[dr2_parallax_over_error] );
			double phot_g_mean_mag = atof( line[dr2_phot_g_mean_mag] );
			if( parallax_over_error < 2. )
			{
				//printf( "X %f %f %f %f %f\n", ra, dec, parallax, phot_g_mean_mag, parallax_over_error );
				break;
				//I'm sure negative parallaxes are useful for science somewhere.
				//They're not useful for pretty pictures.
			}

			//if( phot_g_mean_mag < 4.5 )
			{
				//printf( "%f %f %f %f %f\n", ra, dec, parallax, phot_g_mean_mag, parallax_over_error );
				//Let's see if we can find this star in the HIP dataset.
				flat_star s;
				s.rascention_bams =  (( ra  )/360)*4294967295;
				s.declination_bams = (( dec )/180)*2147483647;
				s.parallax_10uas =   (( parallax ) ) * 100;
				s.magnitude_mag1000 = (( phot_g_mean_mag )) * 1000;
				s.bvcolor_mag1000 = (( 0 )) * 1000;
				s.vicolor_mag1000 = (( 0 )) * 1000;
				s.hipparcos_id = 0;


				flat_star * fsp = malloc( sizeof( flat_star ) );
				memcpy( fsp, &s, sizeof( s ) );
				uint32_t rabams = fsp->rascention_bams;
				struct flat_star_linked_list * lnew = malloc( sizeof( struct flat_star_linked_list ) );
				struct flat_star_linked_list * lold = RBHAS( hipset, rabams ) ? RBA( hipset, rabams ) : 0;
				lnew->next = lold;
				if( lold )
				{
					//printf( "%d %d // %d %d\n", lold->data->rascention_bams, lold->data->declination_bams, s.rascention_bams, s.declination_bams );
				}
				lnew->data = fsp;
				RBA( drset, rabams ) = lnew;

				//printf( "%d\n", s.declination_bams );
				//int hipmatch = FindHIPStar( s.rascention_bams, s.declination_bams, s.parallax_10uas, s.magnitude_mag1000 );
			}
//			int FindHIPStar( uint32_t rascention_bams, int32_t declination_bams, int parallax, int mag );
			//typedef struct __attribute__((__packed__)) { uint32_t rascention_bams; int32_t declination_bams; int16_t parallax_10uas; uint16_t magnitude_mag1000; int16_t bvcolor_mag1000; int16_t vicolor_mag1000; uint32_t hipparcos_id; } flat_star;



		} while( 0 );

		field = 0;
		linepl = 0;
		//printf( "EC: %d\n", ec );
		//exit( 0 );
		// DR2 definitions : https://gea.esac.esa.int/archive/documentation/GDR2/Gaia_archive/chap_datamodel/sec_dm_main_tables/ssec_dm_gaia_source.html
	}

	printf( "Both drset and hipset are now populated.\n" );
	//Next step.  Iterate over the hipparcos dataset, finding corresponding stars in DR2.

	int hipcount = 0;
	RBFOREACH( uint32_tfsllptr, hipset, p )
	{
		struct flat_star_linked_list * ll = p->data;
		while( ll )
		{
			flat_star * data = ll->data;
			int bestscore;
			bestscore = 100000;
			flat_star * s = FindDRStar( data->rascention_bams, data->declination_bams, data->parallax_10uas, data->magnitude_mag1000, data->hipparcos_id, 0, &bestscore );
			if( s )
			{
				if( s->hipparcos_id )
				{
					printf( "OVERRIDING STAR\n" );
				}
				s->hipparcos_id = data->hipparcos_id;
			}
			else
			{
				//No matching star.
				if( data->parallax_10uas < 5 || data->magnitude_mag1000 > 10000 )
				{
					printf( "HIPPARCOS STAR %d MARKED FOR DELETION\n", data->hipparcos_id );
				}
				else
				{
					//Otherwise, add this star to the output dataset.
					out_stars = realloc( out_stars, sizeof( flat_star ) * (num_out_stars + 1) );
					memcpy( &out_stars[num_out_stars], data, sizeof( flat_star ) );
					num_out_stars++;
				}
			}
			ll = ll->next;	
			hipcount++;
			if( ( hipcount % 1000 ) == 0 )
			{
				printf( "HIP: %d\n", hipcount );
			}
		}
	}

	//Next we need to correlate colors.
	RBFOREACH( uint32_tfsllptr, dr2set, p )
	{
		struct flat_star_linked_list * ll = p->data;
		while( ll )
		{
			flat_star * data = ll->data;
			if( data->hipparcos_id )
			{
				//We have a corresponding star.
				//XXX TODO PICK UP HERE!!!
			}
		}
	}

}





flat_star *  FindDRStar( uint32_t rascention_bams, int32_t declination_bams, int parallax, int mag, int on_behalf_of_id, int wrap, int * bestscore )
{
	//XXX TODO Handle wrap-around.
	#define BAMWINDOW 800000
	uint32_t targasc = rascention_bams - BAMWINDOW;

	if( wrap == 1 )
	{
		targasc = 0;
	}

	flat_star * beststar = 0;

	if( targasc < 0 )
	{
		beststar = FindDRStar( rascention_bams, declination_bams, parallax, mag, on_behalf_of_id, 1, bestscore );
	}

	if( targasc >= ( (1LL<<32) - BAMWINDOW ) )
	{
		beststar = FindDRStar( rascention_bams, declination_bams, parallax, mag, on_behalf_of_id, 1, bestscore );
	}

	cnrbtree_uint32_tfsllptr_node * approx = cnrbtree_uint32_tfsllptr_get2( drset, targasc, 1 );


	int breakout = 0;
	while( approx && breakout == 0 )
	{
		approx = (cnrbtree_uint32_tfsllptr_node *)cnrbtree_generic_next( (cnrbtree_generic*)drset, (cnrbtree_generic_node *) approx );

		if( !approx ) break;
		struct flat_star_linked_list * fsll = approx->data;

		for( ; fsll ; fsll = fsll->next )
		{
			flat_star * data = fsll->data;


			if( data->rascention_bams > rascention_bams + BAMWINDOW ) { breakout = 1; break; }

			if( data->declination_bams > declination_bams - BAMWINDOW && 
				data->declination_bams < declination_bams + BAMWINDOW )
			{
				int32_t dra = (int32_t)data->rascention_bams - rascention_bams;
				int32_t ddec = (int32_t)data->declination_bams - declination_bams;
				int32_t dpara = (int32_t)data->parallax_10uas - parallax;
				int32_t dmag = (int32_t)data->magnitude_mag1000 - mag;
				int score = IABS(dra) / 10 + IABS(ddec) / 10 + IABS(dpara) * 10 + IABS(dmag);
//				printf( " %d, %d, %d, %d / %d %d, %d, %d\n", data->rascention_bams, data->declination_bams, data->parallax_10uas, data->magnitude_mag1000, rascention_bams, declination_bams, parallax, mag );
//				printf( "    %d %d %d %d (%d)\n", dra, ddec, dpara, dmag, score );
				if( score < *bestscore )
				{
					*bestscore = score;
					beststar = data;
				}
			}
		}
	}

	//flat_star * beststar = 0;
	//int bestscore = 60000;
	//int ret = -1;
	if( !beststar )
	{
		//if( mag < 8000 )
		{
			printf( "Unmatching star (%d)\n", on_behalf_of_id );
			printf( " %d %d, %d, %d\n", rascention_bams, declination_bams, parallax, mag );
		}
		return 0;
	}
	else
	{
		//First, see if this is a repeat-star or something that's already been correlated?
		if( beststar->hipparcos_id && beststar->hipparcos_id == on_behalf_of_id ) return 0;

		if( beststar->hipparcos_id )
		{
			flat_star * fs = RBA( hipsetbyhipid, beststar->hipparcos_id );
			int32_t dra1 = (int32_t)beststar->rascention_bams - rascention_bams;
			int32_t ddec1 = (int32_t)beststar->declination_bams - declination_bams;
			int32_t dpara1 = (int32_t)beststar->parallax_10uas - parallax;
			int32_t dmag1 = (int32_t)beststar->magnitude_mag1000 - mag;
			int score1 = IABS(dra1) / 10 + IABS(ddec1) / 10 + IABS(dpara1) * 10 + IABS(dmag1);

			int32_t dra2 = (int32_t)fs->rascention_bams - rascention_bams;
			int32_t ddec2 = (int32_t)fs->declination_bams - declination_bams;
			int32_t dpara2 = (int32_t)fs->parallax_10uas - parallax;
			int32_t dmag2 = (int32_t)fs->magnitude_mag1000 - mag;
			int score2 = IABS(dra2) / 10 + IABS(ddec2) / 10 + IABS(dpara2) * 10 + IABS(dmag2);

			printf( "Duplicate star (%d) on behalf of %d\n",beststar->hipparcos_id, on_behalf_of_id );
			printf( " %d, %d, %d, %d / %d, %d, %d, %d // %d\n", beststar->rascention_bams, beststar->declination_bams, beststar->parallax_10uas, beststar->magnitude_mag1000, rascention_bams, declination_bams, parallax, mag, score1 );
			printf( " competes with %d %d %d %d (%d) // %d\n", fs->rascention_bams, fs->declination_bams, fs->parallax_10uas, fs->magnitude_mag1000, fs->hipparcos_id, score2 );
			if( score2 < score1 )
			{
				printf( "OVERRIDING\n" );
				return beststar;
			}
			return 0;
		}
		else
		{
			//Matching star!
			return beststar;
		}
	}
	//cnrbtree_uint32_tfsllptr * hipset;
	//cnrbtree_##key_t##data_t##_get2( cnrbtree_##key_t##data_t * tree, key_t key, int approx );
	
}





