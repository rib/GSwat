#!/bin/bash

LC_PREFIX_DEFAULT="gswat"
#UC_PREFIX_DEFAULT is implicit
CC_PREFIX_DEFAULT="GSwat"

echo "This will run your through some quick steps"
echo "to generate an outline for your new class"
echo


while test "$FILESTEM" == ""
do
    echo "Enter a filename stem (filename without a .c or .h extension)"
    read -p " > " -e FILESTEM
done
echo "filename stem = $FILESTEM"



echo
echo "Enter a project wide, lowercase symbol prefix:"
read -p "[$LC_PREFIX_DEFAULT] >" -e LC_PREFIX
if test "$LC_PREFIX" == ""; then
    LC_PREFIX=$LC_PREFIX_DEFAULT
fi
echo "lowercase symbol prefix=$LC_PREFIX"


UC_PREFIX_DEFAULT=$(echo -n $LC_PREFIX|tr a-z A-Z)
echo
echo "Enter a project wide, UPPERCASE symbol prefix:"
read -p "[$UC_PREFIX_DEFAULT] >" -e UC_PREFIX
if test "$UC_PREFIX" == ""; then
    UC_PREFIX=$UC_PREFIX_DEFAULT
fi
echo "UPPERCASE symbol prefix=$LC_PREFIX"

echo
echo "Enter a project wide, CamelCase symbol prefix:"
read -p "[$CC_PREFIX_DEFAULT] >" -e CC_PREFIX
if test "$CC_PREFIX" == ""; then
    CC_PREFIX=$CC_PREFIX_DEFAULT
fi
echo "CamelCase symbol prefix=$CC_PREFIX"

echo
while test "$LC_CLASS_NAME" == ""
do
    echo "Enter a lowercase class name:"
    read -p " > " -e LC_CLASS_NAME
done
echo "lowercase class name = $LC_CLASS_NAME"


UC_CLASS_NAME_DEFAULT=$(echo $LC_CLASS_NAME|tr a-z A-Z)
echo
echo "Enter an UPPERCASE class name:"
read -p "[$UC_CLASS_NAME_DEFAULT] >" -e UC_CLASS_NAME
if test "$UC_CLASS_NAME" == ""; then
    UC_CLASS_NAME=$UC_CLASS_NAME_DEFAULT
fi
echo "UPPERCASE class name=$UC_CLASS_NAME"

echo
while test "$CC_CLASS_NAME" == ""
do
    echo "Enter an CamelCase class name:"
    read -p " > " -e CC_CLASS_NAME
done
echo "CamelCase class name=$CC_CLASS_NAME"

cp g-object-class.c ${FILESTEM}.c
sed -i "s/my-object.h/${FILESTEM}.h/g" ${FILESTEM}.c
sed -i "s/my_object/${LC_PREFIX}_${LC_CLASS_NAME}/g" ${FILESTEM}.c
sed -i "s/MY_OBJECT/${UC_PREFIX}_${UC_CLASS_NAME}/g" ${FILESTEM}.c
sed -i "s/MY_TYPE_OBJECT/${UC_PREFIX}_TYPE_${UC_CLASS_NAME}/g" ${FILESTEM}.c
sed -i "s/MY_IS_OBJECT/${UC_PREFIX}_IS_${UC_CLASS_NAME}/g" ${FILESTEM}.c
sed -i "s/MyObject/${CC_PREFIX}${CC_CLASS_NAME}/g" ${FILESTEM}.c

cp g-object-class.h ${FILESTEM}.h
sed -i "s/my_object/${LC_PREFIX}_${LC_CLASS_NAME}/g" ${FILESTEM}.h
sed -i "s/MY_OBJECT/${UC_PREFIX}_${UC_CLASS_NAME}/g" ${FILESTEM}.h
sed -i "s/MyObject/${CC_PREFIX}${CC_CLASS_NAME}/g" ${FILESTEM}.h
sed -i "s/MY_TYPE_OBJECT/${UC_PREFIX}_TYPE_${UC_CLASS_NAME}/g" ${FILESTEM}.h
sed -i "s/MY_IS_OBJECT/${UC_PREFIX}_IS_${UC_CLASS_NAME}/g" ${FILESTEM}.h

echo "Done!"

