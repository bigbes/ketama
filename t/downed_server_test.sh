function check_count() {
    if [ $1 = 0 ]; then
        printf "\033[32;1m %-10s \033[0m\n" "OK"
    else
        PC=$[$1 * 100 / $2]
        printf "\033[33;1m %-10s \033[0m %d items distributed (%d%s)\n" "WARNNING" $1 $PC "%"
    fi
}

echo "Downed server test, setting up.."

printf "%-50s\033[32;1m %-10s \033[0m\n" "  Running on original set of servers.. " "OK"
./ketama_test ketama.servers > original_mapping

# set up downed server
head -n `wc -l ketama.servers | awk '{print $1-1}'` ketama.servers > ketama.servers.minus.one
DOWNED_SERVER=`tail -n 1 ketama.servers | awk '{print $1}'`

# set up new server
NEW_SERVER=999.999.999:11211
cp ketama.servers ketama.servers.plus.one
echo "$NEW_SERVER\t300" >> ketama.servers.plus.one

NUM_ORIGIN_ITEMS=`wc -l original_mapping | awk '{print $1}'`

printf "%-50s" "  Running on original - 1 set of servers.. "
./ketama_test ketama.servers.minus.one > minus_one_mapping
MINUS_ONE_MOVED_ITEMS=`diff -y --suppress-common-lines original_mapping minus_one_mapping | grep -c -v "${DOWNED_SERVER}"`
check_count $MINUS_ONE_MOVED_ITEMS $NUM_ORIGIN_ITEMS

printf "%-50s" "  Running on original + 1 set of servers.. "
./ketama_test ketama.servers.plus.one > plus_one_mapping
PLUS_ONE_MOVED_ITEMS=`diff -y --suppress-common-lines original_mapping plus_one_mapping | grep -c -v "${NEW_SERVER}"`
check_count $PLUS_ONE_MOVED_ITEMS $NUM_ORIGIN_ITEMS

rm -rf ketama.servers.minus.one ketama.servers.plus.one original_mapping minus_one_mapping plus_one_mapping
echo "Finished downed server test."
