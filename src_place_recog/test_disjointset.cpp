#include <iostream>

#include "../utils/DisjointSet.h"
using namespace mp;
using namespace std;
int main()
{
    std::cout << "hello\n";
    mp::DisjointSetForest<float> ds;
    ds.add_element( 0, 213.1 );
    // ds.add_element( 0, 21.1 );
    ds.add_element( 1, 4.44 );
    ds.add_element( 2, -0.4 );
    ds.add_element( 3, -120.4 );
    ds.union_sets( 0, 1);
    cout << "exist(2) = " << ds.exists( 2 ) << endl;
    cout << "exist(56) = " << ds.exists( 56 ) << endl;
    // ds.union_sets( 0, 56);
    std::cout << "element count: " << ds.element_count() << std::endl;
    cout << "set_count: " << ds.set_count() << endl;


    for( int i=0 ; i<ds.element_count() ; i++ ) {
        cout << "element#" << i << "  find_set=" << ds.find_set( i ) ;
        cout << "  value=" << ds.value_of( i );
        cout << endl;
    }
}
