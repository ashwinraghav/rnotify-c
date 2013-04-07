for (( i = 1; i <= 50000; i++ ))      ### Outer for loop ###
do
                echo "asd" >>  1/$i
                echo "asd" >>  2/$i
                echo "asd" >>  3/$i
                echo "asd" >>  4/$i
                let "remainder = $i % 10000"
                if [ "$remainder" == 0 ]
                then
                        echo $i
                        #sleep 1
                fi
done
