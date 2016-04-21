<!DOCTYPE html>
<html lang="ja">
<head>
<meta charset="UTF-8">
<title>IPアドレスから国を判別するサンプル</title>
</head>
<body>
<h1>IPアドレスから国を判別するサンプル</h1>
<hr>
<?php
	require_once(dirname(__FILE__).DIRECTORY_SEPARATOR.'CountryCode.inc');
	
	$cc = new CountryCode();
	$ip_addr = $_SERVER['REMOTE_ADDR'];
	
	if ( ($country_code = $cc->DetectCountryCode($ip_addr)) !== false ) {
		$answer = 'IPアドレスから割り出した国名コード: <strong style="color:blue;">'.$country_code.'</strong>';
	} else {
		$answer = '<span style="color:red;">IPアドレスに該当する国名コードが見つかりません</span>';
	}
?>
接続元IPアドレス: <strong><?php echo $ip_addr; ?></strong><br>
→<?php echo $answer; ?></strong><br>
</body>
</html>