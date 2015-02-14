#!/sbin/sh

cp /tmp/aroma/freq0.prop /tmp/aroma/freq1.prop;
cp /tmp/aroma/freq0.prop /tmp/aroma/freq2.prop;
cp /tmp/aroma/freq0.prop /tmp/aroma/freq3.prop;

#set max_oc
val0=$(cat /tmp/aroma/freq0.prop | cut -d '=' -f2)

case $val0 in

	1)
	  max_oc0="max_oc0=1497600"
	  ;;
	2)
	  max_oc0="max_oc0=1728000"
	  ;;
	3)
	  max_oc0="max_oc0=1958400"
	  ;;
	4)
	  max_oc0="max_oc0=2265600"
	  ;;
  	5)
	  max_oc0="max_oc0=2342400"
	  ;;
  	6)
	  max_oc0="max_oc0=2419200"
	  ;;
	7)
	  max_oc0="max_oc0=2496000"
	  ;;
	8)
	  max_oc0="max_oc0=2572800"
	  ;;
	9)
	  max_oc0="max_oc0=2649600"
	  ;;
	10)
	  max_oc0="max_oc0=2726400"
	  ;;
	11)
	  max_oc0="max_oc0=2803200"
	  ;;
	12)
	  max_oc0="max_oc0=2880000"
	  ;;
	13)
	  max_oc0="max_oc0=2956800"
	  ;;
esac

val1=$(cat /tmp/aroma/freq1.prop | cut -d '=' -f2)
case $val1 in

	1)
	  max_oc1="max_oc1=1497600"
	  ;;
	2)
	  max_oc1="max_oc1=1728000"
	  ;;
	3)
	  max_oc1="max_oc1=1958400"
	  ;;
	4)
	  max_oc1="max_oc1=2265600"
	  ;;
  	5)
	  max_oc1="max_oc1=2342400"
	  ;;
  	6)
	  max_oc1="max_oc1=2419200"
	  ;;
	7)
	  max_oc1="max_oc1=2496000"
	  ;;
	8)
	  max_oc1="max_oc1=2572800"
	  ;;
	9)
	  max_oc1="max_oc1=2649600"
	  ;;
	10)
	  max_oc1="max_oc1=2726400"
	  ;;
	11)
	  max_oc1="max_oc1=2803200"
	  ;;
	12)
	  max_oc1="max_oc1=2880000"
	  ;;
	13)
	  max_oc1="max_oc1=2956800"
	  ;;
esac

val2=$(cat /tmp/aroma/freq2.prop | cut -d '=' -f2)
case $val2 in

	1)
	  max_oc2="max_oc2=1497600"
	  ;;
	2)
	  max_oc2="max_oc2=1728000"
	  ;;
	3)
	  max_oc2="max_oc2=1958400"
	  ;;
	4)
	  max_oc2="max_oc2=2265600"
	  ;;
  	5)
	  max_oc2="max_oc2=2342400"
	  ;;
  	6)
	  max_oc2="max_oc2=2419200"
	  ;;
	7)
	  max_oc2="max_oc2=2496000"
	  ;;
	8)
	  max_oc2="max_oc2=2572800"
	  ;;
	9)
	  max_oc2="max_oc2=2649600"
	  ;;
	10)
	  max_oc2="max_oc2=2726400"
	  ;;
	11)
	  max_oc2="max_oc2=2803200"
	  ;;
	12)
	  max_oc2="max_oc2=2880000"
	  ;;
	13)
	  max_oc2="max_oc2=2956800"
	  ;;
esac

val3=$(cat /tmp/aroma/freq3.prop | cut -d '=' -f2)
case $val3 in

	1)
	  max_oc3="max_oc3=1497600"
	  ;;
	2)
	  max_oc3="max_oc3=1728000"
	  ;;
	3)
	  max_oc3="max_oc3=1958400"
	  ;;
	4)
	  max_oc3="max_oc3=2265600"
	  ;;
  	5)
	  max_oc3="max_oc3=2342400"
	  ;;
  	6)
	  max_oc3="max_oc3=2419200"
	  ;;
	7)
	  max_oc3="max_oc3=2496000"
	  ;;
	8)
	  max_oc3="max_oc3=2572800"
	  ;;
	9)
	  max_oc3="max_oc3=2649600"
	  ;;
	10)
	  max_oc3="max_oc3=2726400"
	  ;;
	11)
	  max_oc3="max_oc3=2803200"
	  ;;
	12)
	  max_oc3="max_oc3=2880000"
	  ;;
	13)
	  max_oc3="max_oc3=2956800"
	  ;;
esac

#set optimization level
val4=$(grep item.0.5 /tmp/aroma/mods.prop | cut -d '=' -f2)

case $val4 in
	1)
	  l2_opt="l2_opt=1"
	  ;;
	2)
	  l2_opt="l2_opt=0"
	  ;;
esac

echo "cmdline = console=ttyHSL0,115200,n8 androidboot.hardware=hammerhead user_debug=31 msm_watchdog_v2.enable=1 androidboot.selinux=permissive" $max_oc0 $max_oc1 $max_oc2 $max_oc3 $l2_opt > /tmp/cmdline.cfg
