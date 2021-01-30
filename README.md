# MisakiEQ_For_Arduino
This project is can display Japan map and earthquake information in Arduino MEGA. This app is available only Japan.

This text is the same as the Readme in the EQW folder.

MisakiEQ For Arduino 
開発者:水咲(みさき)

主な機能
外部アプリケーションとCOM通信することによって緊急地震速報受信時に地図と場所を表示してくれます。
今後のアップデートによって気象庁発表のデータと照らし合わせる事が可能です。

前提条件
ILI9481チップを搭載した65536色の320x480液晶ディスプレイをArduino MEGAにドッキングさせること。
Arduinoのアプリケーションを所持していること。
Weather News社のThe Last 10-Second及びWeather News社のプレミアム会員アカウントと
W Sparkle様の緊急地震速報アラームのアプリケーションを所持していること。
緊急地震速報アラームには各種設定にて「基本設定2」→「その他の設定」→「速報受信時にコマンド実行」の入力欄にMisakiEQWの実行ファイルと共に
「このディレクトリのパス\MisakiEQW.exe <名称> <猶予時間> <推定震度> <震央> <深さ> <マグニチュード> <最大震度> <報> <緯度> <経度> <発生年月日> <発生時刻> <発令年月日> <発令時刻> <到達年月日> <到達時刻> <最終報> <津波発生の可能性> <キャンセル報> <訓練>」
を入力していること。分からなければMisakiEQW.exeを一度起動してパスを確認してね。
「第〇報をすべて実行」にチェックが付いていること。
(必要に合わせて)PCとネットワークが常時稼働していること。

MisakiEQW.exeについて
そのまま起動するとエラーが出ますが、
MisakiEQW.exeを実行すると生成されるファイル「MisakiEQW.ini」には送信するCOM番号が書かれています。
ArduinoのCOMポート番号に合わせて変更してください。もし削除されると次回実行時に生成されます。
既定値は4です。
またこのアプリケーションはArduinoにデータを送信する役割を担っています。これがないと表示されないとても重要なものです。

Arduino側はこのままコンパイルするとエラーが出るはずなので、コンパイルエラーに困ったときは
libフォルダ内のLCDWIKI_XXXを
「C:\Users\ユーザー名\Documents\Arduino\libraries」
にドラッグアンドドロップ。

制作者Twitter:@0x7FF
PayPalにて寄付を募っています: http://paypal.me/Blueplanet256
