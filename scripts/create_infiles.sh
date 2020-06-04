#!/bin/bash


USAGE="Usage: create_infiles.sh diseasesFile countriesFile input_dir numFilesPerDirectory numRecordsPerFile"
NAMES=("Bill" "Duke" "Paul" "Oscar" "Red" "Dave" "Paul" "Joe")
NAMLEN=${#NAMES[@]}
SURNAMES=("Evans" "Ellington" "Desmond" "Peterson" "Garland" "Brubeck" "Chambers" "Pass")
SURLEN=${#SURNAMES[@]}


# Check number of arguements

if [ $# -ne 5 ] 
then
    echo $USAGE
    exit 1
fi


# Set up params

DSSF=$1			# diseasesFile
CTRF=$2			# countriesFile
INDIR=$3		# input_dir
FILESPERDIR=$4	# numFilesPerDirectory
RECSPERFILE=$5	# numRecordsPerFile


# Create input_dir

mkdir $INDIR

if [ $? -ne 0 ] 
then
	exit 2
fi


# Read countriesFile into an indexed array

dn=0		# number of diseases

while read disease
do
	diseases[$dn]=$disease
	dn=`expr $dn + 1`
	
done < $DSSF	


# Create a subdirectory for each country in countriesFile

ID=1

while read country 
do
	mkdir $INDIR/$country
	
	# Create $FILESPERDIR subdirectories 
	
	for i in `seq 1 $FILESPERDIR`
	do
		d=`expr $RANDOM % 30 + 1`
		m=`expr $RANDOM % 12 + 1`
		y=`expr $RANDOM % 6 + 2015`
		
		touch $INDIR/$country/$d-$m-$y
		
		# Insert $RECSPERFILE records in each file
		
		for j in `seq 1 $RECSPERFILE`
		do
			NOM=${NAMES[`expr $RANDOM % $NAMLEN`]}
			PRENOM=${SURNAMES[`expr $RANDOM % $SURLEN`]}
			DIS=${diseases[`expr $RANDOM % $dn`]}
			AGE=`expr $RANDOM % 120 + 1`
			
			echo "$ID ENTER $NOM $PRENOM $DIS $AGE" >> $INDIR/$country/$d-$m-$y
			
			ID=`expr $ID + 1`
			
		done
		
	done
	
done < $CTRF



