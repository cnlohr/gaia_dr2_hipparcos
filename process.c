#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char ** CNOVRSplitStrings( const char * line, char * split, char * white, int merge_fields, int * elementcount )
{
	if( elementcount ) *elementcount = 0;

	if( !line || strlen( line ) == 0 )
	{
		char ** ret = malloc( sizeof( char * )  );
		*ret = 0;
		return ret;
	}

	int elements = 1;
	char ** ret = malloc( elements * sizeof( char * )  );
	int * lengths = malloc( elements * sizeof( int ) ); 
	int i = 0;
	char c;
	int did_hit_not_white = 0;
	int thislength = 0;
	int thislengthconfirm = 0;
	int needed_bytes = 1;
	const char * lstart = line;
	do
	{
		int is_split = 0;
		int is_white = 0;
		char k;
		c = *(line);
		for( i = 0; (k = split[i]); i++ )
			if( c == k ) is_split = 1;
		for( i = 0; (k = white[i]); i++ )
			if( c == k ) is_white = 1;

		if( c == 0 || ( ( is_split ) && ( !merge_fields || did_hit_not_white ) ) )
		{
			//Mark off new point.
			lengths[elements-1] = (did_hit_not_white)?(thislengthconfirm + 1):0; //XXX BUGGY ... Or is bad it?  I can't tell what's wrong.  the "buggy" note was from a previous coding session.
			ret[elements-1] = (char*)lstart + 0; //XXX BUGGY //I promise I won't change the value.
			needed_bytes += thislengthconfirm + 1;
			elements++;
			ret = realloc( ret, elements * sizeof( char * )  );
			lengths = realloc( lengths, elements * sizeof( int ) );
			lengths[elements-1] = 0;
			lstart = line;
			thislength = 0;
			thislengthconfirm = 0;
			did_hit_not_white = 0;
			line++;
			continue;
		}

		if( !is_white && ( !(merge_fields && is_split) ) )
		{
			if( !did_hit_not_white )
			{
				lstart = line;
				thislength = 0;
				did_hit_not_white = 1;
			}
			thislengthconfirm = thislength;
		}

		if( is_white )
		{
			if( did_hit_not_white ) 
				is_white = 0;
		}

		if( did_hit_not_white )
		{
			thislength++;
		}
		line++;
	} while ( c );

	//Ok, now we have lengths, ret, and elements.
	ret = realloc( ret, ( sizeof( char * ) + 1 ) * elements  + needed_bytes );
	char * retend = ((char*)ret) + ( (sizeof( char * )) * elements);
	int lensum1 = 0;
	for( i = 0; i < elements; i++ )
	{
		int len = lengths[i];
		lensum1 += len + 1;
		memcpy( retend, ret[i], len );
		retend[len] = 0;
		ret[i] = (i == elements-1)?0:retend;
		retend += len + 1;
	}
	if( elementcount && elements ) *elementcount = (thislength==0)?(elements-1):elements;
	free( lengths );
	return ret;
}

char line[8192];
int linepl;
int main( int argc, char ** argv )
{
	while( !feof( stdin ) )
	{
		int c;
		linepl = 0;
		while( ( c = fgetc( stdin ) ) != EOF )
		{
			if( c == '\n' ) break;
			line[linepl++] = c;
		}
		line[linepl] = 0;
		int elementcount;
		char ** dat = CNOVRSplitStrings( line, ",", " ", 0, &elementcount );
		if( elementcount < 50 ) continue;
		float gmag = atof( dat[50] );
		if( gmag < 13.5 && gmag > 0 )
		{
			printf( "%s\n", line );
		}
//		printf( "%s\n", dat[50] );
		free( dat );
	}
	//char ** CNOVRSplitStrings( const char * line, char * split, char * white, int merge_fields, int * elementcount )
}


