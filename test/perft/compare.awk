#!/bin/awk -f

BEGIN   { 
    FS="[ \t\n]+"
    #FS=" "
    print "Begin" 
}

NR==FNR {
    print $1 " " $2
    depth[$1]
    nodes[$2]
    captures[$3]
    ep[$4]
    next
}
$1 in captures {
    print $1
}
$1 in nodes {
    print $1
}
END     { 
    print "End" 
}
