bin_PROGRAMS = subscribe_45g_server cal_sign

subscribe_45g_server_SOURCES= subscribe_server.cpp subscribeReq.cpp subscribe_config.cpp
cal_sign_SOURCES= cal_sign.cpp

AM_CPPFLAGS= -g -Wall -Werror -I@top_builddir@ \
	-I${prefix}/include/zc_jkpt_tools -I${JANSSON_DIR}/include -I${OPENSSL_DIR}/include

AM_LDFLAGS = -L${prefix}/lib -L${JANSSON_DIR}/lib -L${OPENSSL_DIR}/lib -lzc_jkpt_pub -ldl -lpthread -ljansson -lcrypto
