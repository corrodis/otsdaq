source /data/ups/setup
setup mrb
setup git
#setup gitflow
source ${PWD}/localProducts_*/setup
#mrb updateSource
mrbsetenv
export USER_DATA=${PWD}/NoGitData
export CETPKG_J=12
ulimit -c unlimited
alias StartOTS.sh=${PWD}/srcs/otsdaq/tools/StartOTS.sh
alias kx='killall -9 xdaq.exe; killall -9 mpirun; killall -9 mf_rcv_n_fwd'
