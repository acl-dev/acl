#!/bin/sh

os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./https_request -f "../libmbedtls_all.dylib" -H intl.iqiyi.com -s "61.220.62.165|443" -S \
		-U "/control/mypage?app_k=2000100237b087b83a2f321235151b14&app_lm=intl&app_t=i18nvideo&app_v=2.7.0&dev_os=13.3.1&dev_ua=iPhone9%2C3&lang=zh_tw&mod=intl&net_sts=1&platform_id=1071&psp_cki=29pO8jPz0EX6RNNQeVkCi1wqHBQVLugGpm2JRpx7do2DVPrn6GdAUvPF9q8HuscEFUZf6&psp_status=-1&psp_uid=30100071120&qyid=66772523dbdea275d8ed8822436debf1d1c2eeab&req_sn=1593659595928&req_times=1&secure_p=iPhone_i18n&secure_v=1&timezone=GMT%2B8"
else
	./https_request -f "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -s www.sina.com.cn:443 -S
fi
