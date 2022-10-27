#!/usr/bin/bash

curl 'https://www.fordtechservice.dealerconnection.com/vdirs/wds/PCMReprogram/DSFM_DownloadFile.asp' \
      -X POST -H 'User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:103.0) Gecko/20100101 Firefox/103.0' \
      -H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8' \
      -H 'Accept-Language: en-US,en;q=0.5' -H 'Accept-Encoding: gzip, deflate, br' \
      -H 'Content-Type: application/x-www-form-urlencoded' \
      -H 'Origin: https://www.motorcraftservice.com' \
      -H 'DNT: 1' \
      -H 'Connection: keep-alive' \
      -H 'Referer: https://www.motorcraftservice.com/' \
      -H 'Cookie: ASPSESSIONIDCUQQRTTC=PIMPBBFBOFOBIIFHDJBBKBFL; dtCookie=v_4_srv_27_sn_18F2B7684336F24E7B97535FBABDD357_perc_100000_ol_0_mul_1_app-3A79dff4109f3df103_1_rcs-3Acss_0; ASPSESSIONIDQUTQQTTB=LPKDFBFBOOAPHDFGHDMDINIF; ASPSESSIONIDAWQRTTTD=GEPJNPEBKNAOBEBCOKIMAIKO; rxVisitor=16668844549918SA75RH9F31OROGJFB2AMK5PE9U83152; dtPC=27$84454988_208h-vATRUKPQHORVHOTMINRCJCMMFLVAROPPJ-0e0; rxvt=1666886255017|1666884454992; dtLatC=3; dtSa=-' \
      -H 'Upgrade-Insecure-Requests: 1' \
      -H 'Sec-Fetch-Dest: document' \
      -H 'Sec-Fetch-Mode: navigate' \
      -H 'Sec-Fetch-Site: cross-site' \
      -H 'Sec-Fetch-User: ?1' \
      -H 'TE: trailers' \
      --data-raw 'filename=SW-N3M5EK001' \
      --output 'SW-N3M5EK001.zip'