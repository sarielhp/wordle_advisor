/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 * wordle.C -
 *     
\*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include  <stdlib.h>
#include  <stdio.h>
#include  <assert.h>
#include  <string.h>

#include  <algorithm>
#include  <fstream>
#include  <iostream>
#include  <string>
#include  <vector>


using namespace std;

/*--- Constants ---*/


/*--- Start of Code ---*/

void  toUpperCase( const string  & s, string & o )
{
    o.resize(s.size());

    std::transform( s.begin(), s.end(), o.begin(), ::toupper);
}


/////////////////////////////////////////////////
class  Dictionary
{
    vector<string>  words;
public:
    void   read( const char   * fl );
    unsigned  size() { return words.size(); }
    const string  & word( int  pos ) const;
};

const string  & Dictionary::word( int  id ) const
{
    assert( ( id >= 0 )  &&  ( id < (int)words.size() ) );
    return words[ id ];
}
    

void   Dictionary::read( const char   * fl )
{
  string  line;

  ifstream  myfile( fl );

  if ( ! myfile.is_open()) {
      fprintf( stderr, "Unable to open file: [%s]\n", fl );
      exit( -1 );
  }

  string  oline;
  while ( getline (myfile,line) ) { 
      toUpperCase( line, oline );
      words.push_back( oline );
  }

  myfile.close();
}


/////////////////////////////////////////////////////////////////////////
// Pattern is a string, where each letter might be prefixed with one
// of the three characters:
//    -c   The letter c does not appear in the desired word.
//    +c   The letter c appears in the desired word, but not in this
//         location
//    !c   The letter c appears in the string in this location.
/////////////////////////////////////////////////////////////////////////

void   pattern_usage()
{
    cout << "Pattern is a string, where each letter must be prefixed with\n"
        " one of the three characters:\n"
        "    -c   The letter c does not appear in the desired word.\n"
        "    +c   The letter c appears in the desired word, but not in this"
        " location\n"
        "    !c   The letter c appears in the string in this location..\n"
        "\n"
        "Examples:\n"
        "!A+B-O+D-E     The desired word starts with an A, contains B "
        " somewhere, etc...\n"
        "\n";
    exit( -1 );
}

void  usage()
{
    cout << "Wordle advisor 0.1, 01/08/2022, by Sariel Har-Peled\n\n"
         << "\t./wordle_advisor  [pattern1] [pattern2]...\n\n";
    pattern_usage();
}


void   Error( const char  * msg )
{
    fprintf( stderr, "%s\n", msg );
    exit( -1 );
}


void   pattern_check( string  & pattern )
{
    if  ( pattern.length() == 0 )
        Error( "Error: Empty pattern!" );
    
    if  ( ( pattern.length() % 2) != 0 )
        Error( "Pattern must have even length" );

    for  ( auto i = 0; i < (int)pattern.length(); i += 2 ) {
        char  c = pattern[ i ];
        if  ( ( c != '-' )  &&  ( c != '+' )  &&  ( c != '!' ) ) {
            fprintf( stderr, "The pattern [%s] is invalid!\n"
                     "Letter at location %d should be either -/+/!\n",
                     pattern.c_str(), 2*i + 1 );
            exit( -1 );
        }
    }    
}


bool  is_no_repeated_letter( const  string  & s )
{
    int  len = (int)s.length();
    
    for  ( int  i = len -1; i > 0; i-- )
        for  ( int  j = i-1; j >= 0; j-- )
            if  ( s[ i ] == s[j ] )
                return  false;

    return  true;        
}

                      
bool   is_contains( const string  & word, const char  c )
{
    for  ( auto  cs : word ) 
        if  ( cs == c )
            return  true;

    return  false;
}


bool   is_pattern_match( const string   & pattern, const string  & word )
{
    for  ( unsigned i = 0; i < pattern.length(); i += 2 ) {
        unsigned int  loc = i / 2;
        char  cmd = pattern[ i ];
        char  c = pattern[i + 1 ];

        if  ( cmd == '!' )  {
            if  ( loc >= word.length() ) 
                return  false;
            if  ( word[ loc ] != c ) 
                return  false;
            continue;
        }
        if  ( cmd == '+' ) {
            if  ( ! is_contains( word, c ) )                
                return  false;
            if  ( loc >= word.length() )
                continue;
            if  ( word[ loc ] == c )
                return  false;
            continue;
        }
        assert( cmd == '-' );
        
        if  ( is_contains( word, c ) )                
            return  false;        
    }

    return  true;    
}



/////////////////////////////////////////////////////////////////////////

class   FrquencyTable
{
    vector<int>  freq;

public:
    FrquencyTable() {
        freq.resize( 'Z' - 'A' + 1 );
        for  ( auto  & en : freq )
            en = 0;
    }
    
    void  count( const string  & s )
    {
        for  ( char  c : s ) {
            if  ( ( c < 'A' )  ||  ( c > 'Z' ) )
                continue;
            freq[ (int)(c - 'A') ]++;
        }
    }

    int  get_score( const string  & s ) {
        int  sc = 0;
        for  ( char  c : s ) {
            if  ( ( c < 'A' )  ||  ( c > 'Z' ) )
                continue;
            sc += freq[ (int)(c - 'A') ];
        }
        return  sc;        
    }    
};


/////////////////////////////////////////////////////////////////////////

class  WLWord
{
    int  id;
    int  score;

public:
    WLWord() {
        id = -1;
        score = 0;
    }

    WLWord( int  _id ) {
        id = _id;
        score = 0;
    }

    int   get_id() const { return  id; }
    void  set_score( int  _score )  { score = _score; }
    int   get_score() const { return  score; }
};

struct WLWord_decreasing
{
    inline bool operator() (const WLWord& wa, const WLWord& wb )
    {
        return  wa.get_score() > wb.get_score();
    }
};

class  WordList
{
private:
    Dictionary  & dict;
    vector<WLWord>  ids;

public:
    WordList( Dictionary   & _dict ) : dict( _dict )
    {
        for  ( unsigned  i = 0; i < dict.size(); i++ )
            ids.push_back( WLWord(i ) );
    }
    
    void  del_words_with_repaated_letters();
    void  sort_by_frequence_score();
    void  output_top( int  top );
    void  del_words_not_matching_pattern( const string  & pat );
        
};
    
void   WordList::del_words_with_repaated_letters()
{
    vector<WLWord>  old_ids;

    old_ids = ids;
    ids.resize( 0 );
    for  ( unsigned  i = 0; i < old_ids.size(); i++ ) {
        WLWord  & word = old_ids[ i ];
        int  id = word.get_id();
        string str = dict.word( id );
        if  ( is_no_repeated_letter( str ) )
            ids.push_back( word );        
    }    
}

void  WordList::sort_by_frequence_score()
{
    FrquencyTable  ft;

    for  ( unsigned  i = 0; i < ids.size(); i++ ) {
        WLWord  & word = ids[ i ];
        ft.count( dict.word( word.get_id() ) );
    }
    for  ( unsigned  i = 0; i < ids.size(); i++ ) {
        WLWord  & word = ids[ i ];
        word.set_score( ft.get_score( dict.word( word.get_id() ) ) );
    }
    std::sort(ids.begin(), ids.end(), WLWord_decreasing() );
}

void  WordList::output_top( int  top )
{
    if  ( (int)ids.size() < top )
        top  = ids.size();
    for  ( int  ind = 0; ind < top; ind++ ) {
        string  str = dict.word( ids[ ind ].get_id() );
        printf( "%02d: %s     score: %d\n",
                ind + 1, str.c_str(), ids[ ind ].get_score() );
    }        
}

void  WordList::del_words_not_matching_pattern( const string  & pat )
{
    vector<WLWord>  old_ids;

    old_ids = ids;
    ids.resize( 0 );
    for  ( unsigned  i = 0; i < old_ids.size(); i++ ) {
        WLWord  & word = old_ids[ i ];
        int  id = word.get_id();
        string w = dict.word( id );
        if  ( is_pattern_match( pat, w ) )
            ids.push_back( word );        
    }    
}

/////////////////////////////////////////////////////////////////////////

int  main( int  argc, char  ** argv )
{
    Dictionary  dict;
    dict.read( "words_5.txt" );

    WordList wl( dict );
    wl.del_words_with_repaated_letters();

    for  ( int  i = 1; i < argc; i++ ) 
        if  ( ( strcasecmp( argv[ i ], "-h" ) == 0 )
              || ( strcasecmp( argv[ i ], "/h" ) == 0 ) )
            usage();

    for  ( int  i = 1; i < argc; i++ ) {
        string  pat( argv[ i ] );
        pattern_check( pat );
        cout << "pattern : " << pat << "\n";
        wl.del_words_not_matching_pattern( pat );
    }
    wl.sort_by_frequence_score();
    wl.output_top( 10 );
    
    return  0;
}

/* wordle.C - End of File ------------------------------------------*/
