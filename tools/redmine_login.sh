
####################################### start redmine login code
####################################### start redmine login code
####################################### start redmine login code
####################################### start redmine login code
#
# urlencode -- encode special characters for post/get arguments
#
urlencode() {
   perl -pe 'chomp(); s{\W}{sprintf("%%%02x",ord($&))}ge;' "$@"
}
site=https://cdcvs.fnal.gov/redmine
listf=/tmp/list_p$$
cookief=/tmp/cookies_p$$
rlverbose=${rlverbose:=false}
trap 'rm -f /tmp/postdata$$ /tmp/at_p$$ $cookief $listf*' EXIT
#
# login form
#
do_login() {
	get_passwords
	get_auth_token "${site}/login"
	post_url  \
       "${site}/login" \
       "back_url=$site" \
       "authenticity_token=$authenticity_token" \
       "username=`echo $user | urlencode`" \
       "password=`echo $pass | urlencode`" \
       "login=Login ?" 
	if grep '>Sign in' $listf > /dev/null;then
		echo
        echo -e "redmine_login.sh [${LINENO}]  \t Login failed."
		unset user #force new login attempt
		unset pass
		LOGIN_WORKED=0
        false
	else
		LOGIN_WORKED=1
        true
	fi
}
get_passwords() {
	
   	case "x${user-}y${pass-}" in
   	xy)
       	if [ -r   ${REDMINE_AUTHDIR:-.}/.redmine_lib_passfile ];then 
	   		read -r user pass < ${REDMINE_AUTHDIR:-.}/.redmine_lib_passfile
       	else
	   
			printf "Enter your Services username: "
			read user

			#user=$USER
			stty -echo
			printf "Services password for $user: "
			read pass
			stty echo
       	fi;;
    esac
}
get_auth_token() {
    authenticity_token=`fetch_url "${1}" |
                  tee /tmp/at_p$$ |
                  grep 'name="authenticity_token"' |
                  head -1 |
                  sed -e 's/.*value="//' -e 's/".*//' | 
                  urlencode `
}
#
# fetch_url -- GET a url from a site, maintaining cookies, etc.
#
fetch_url() {
     wget \
        --no-check-certificate \
	--load-cookies=${cookief} \
        --referer="${lastpage-}" \
	--save-cookies=${cookief} \
	--keep-session-cookies \
	-o ${debugout:-/dev/null} \
	-O - \
	"$1"  | ${debugfilter:-cat}
     lastpage="$1"
}
#
# post_url POST to a url maintaining cookies, etc.
#    takes a url and multiple form data arguments
#    which are joined with "&" signs
#
post_url() {
     url="$1"
     extra=""
     if  [ "$url" == "-b" ];then
         extra="--remote-encoding application/octet-stream"
         shift
         url=$1
     fi
     shift
     the_data=""
     sep=""
     df=/tmp/postdata$$
     :>$df
     for d in "$@";do
        printf "%s" "$sep$d" >> $df
        sep="&"
     done
     wget -O $listf \
        -o $listf.log \
        --debug \
        --verbose \
        $extra \
        --no-check-certificate \
	--load-cookies=${cookief} \
	--save-cookies=${cookief} \
        --referer="${lastpage-}" \
	--keep-session-cookies \
        --post-file="$df"  $url
     if grep '<div.*id=.errorExplanation' $listf > /dev/null;then
        echo "Failed: error was:"
        cat $listf | sed -e '1,/<div.*id=.errorExplanation/d' | sed -e '/<.div>/,$d'
        return 1
     fi
     if grep '<div.*id=.flash_notice.*Success' $listf > /dev/null;then
        $rlverbose && echo "Succeeded"
        return 0
     fi
     # not sure if it worked... 
     $rlverbose && echo "Unknown -- detagged output:"
     $rlverbose && cat $listf | sed -e 's/<[^>]*>//g'
     $rlverbose && echo "-----"
     $rlverbose && cat $listf.log
     $rlverbose && echo "-----"
     return 0
} # post_url

echo
echo -e "redmine_login.sh [${LINENO}]  \t Attempting login... $SKIP_REDMINE_LOGIN"

if [ "x$SKIP_REDMINE_LOGIN" != "x1" ]; then
	do_login https://cdcvs.fnal.gov/redmine
fi 

if [ $LOGIN_WORKED == 0 ]; then
	echo -e "redmine_login.sh [${LINENO}]  \t Check your Fermilab Services name and password!"
	exit 1 
fi

echo -e "redmine_login.sh [${LINENO}]  \t Login successful."

		
####################################### end redmine login code
####################################### end redmine login code
####################################### end redmine login code
####################################### end redmine login code