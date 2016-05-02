<?php
/**
 *
 * IPアドレスから国を判別する
 *
 * @author      kokko
 * @version     1.0
 * @create      2016-04-21
 *
 **/

//タイムゾーン設定
date_default_timezone_set('Asia/Tokyo');

//cidrデータファイルのURL
$ARR_CIDR_DATA_URL = array(
	'delegated-arin-extended-latest'	=>	'http://ftp.arin.net/pub/stats/arin/delegated-arin-extended-latest',
	'delegated-ripencc-latest'			=>	'http://ftp.ripe.net/pub/stats/ripencc/delegated-ripencc-latest',
	'delegated-apnic-latest'			=>	'http://ftp.apnic.net/pub/stats/apnic/delegated-apnic-latest',
	'delegated-lacnic-latest'			=>	'http://ftp.lacnic.net/pub/stats/lacnic/delegated-lacnic-latest',
	'delegated-afrinic-latest'			=>	'http://ftp.afrinic.net/pub/stats/afrinic/delegated-afrinic-latest',
);

//最終更新日時から下記時間[秒]が経過していた場合にリストを再取得
define('COUNTRY_CODE_EXPIRE', 14 * 86400);

//強制的にデータダウンロード/生成を実行
define('COUNTRY_CODE_FORCE_DOWNLOAD', 0);


class CountryCode
{
	//IPアドレス対応表データ
	protected $country_data;
	//リストファイルを保存しておくディレクトリ
	protected $country_data_dir;
	//リストファイル
	protected $country_data_file;

	//エラー格納領域
	var $errors;


	function __construct()
	{
		global $ARR_CIDR_DATA_URL;

		$ret = false;
		$this->errors = array();

		//初期化
		$this->country_data = null;
		$this->country_data_dir = dirname(__FILE__).DIRECTORY_SEPARATOR.'cidr-data'.DIRECTORY_SEPARATOR;
		$this->country_data_file = $this->country_data_dir.'country-data.txt';

		//環境チェック
		if ( !function_exists('file_get_contents') ) { 
			$errors[] = "file_get_contents 関数が存在しません"; 
		} 
		if ( ini_get('allow_url_fopen') != 1 ) { 
			$errors[] = "allow_url_fopen が 1 ではありません";
		}

		//国判別データファイルなどの取得・更新
		if ( !count($this->errors) ) {

			foreach ( $ARR_CIDR_DATA_URL as $cidr_data_name => $cidr_data_url ) {

				$cidr_file_path = $this->country_data_dir . $cidr_data_name;

				if ( !file_exists($cidr_file_path) || !is_file($cidr_file_path) ) {			//CIDRファイルがローカルに存在しなければ
					$cidr_file_create = true;
				} else if ( COUNTRY_CODE_EXPIRE <= time() - filemtime($cidr_file_path) ) {	//ローカルにあるCIDRファイルが古い場合
					$cidr_file_create = true;
				} else {
					$cidr_file_create = COUNTRY_CODE_FORCE_DOWNLOAD;
				}

				if ( $cidr_file_create ) {

					if ( ($cidr_data = file_get_contents($cidr_data_url)) !== false ) {

						if ( file_put_contents($cidr_file_path, $cidr_data) !== false ) {

							chmod($cidr_file_path, 0666);

							//OK!!
							$ret = true;

						} else {
							$this->errors[] = 'CIDRファイルの保存に失敗しました';
							break;
						}

					} else {
						$this->errors[] = 'データ取得に失敗しました';
						break;
					}
						
				}

			}

		}

		//国判別データファイルが古いか存在しなければ作る
		if ( !count($this->errors) ) {

			if ( !file_exists($this->country_data_file) || !is_file($this->country_data_file) ) {
				$country_file_create = true;
			} else if ( COUNTRY_CODE_EXPIRE <= time() - filemtime($this->country_data_file) ) {
				$country_file_create = true;
			} else {
				$country_file_create = COUNTRY_CODE_FORCE_DOWNLOAD;
			}

			if ( $country_file_create ) {

				if ( ($fpo = fopen($this->country_data_file, 'w')) !== false ) {

					$total_count = 0;
					$output_count = 0;

					foreach ( $ARR_CIDR_DATA_URL as $cidr_data_name => $cidr_data_url ) {

						$cidr_file_path = $this->country_data_dir . $cidr_data_name;

						if ( ($fpi = fopen($cidr_file_path, 'r')) !== false ) {

							$total_one_count = 0;
							$output_one_count = 0;

							while ( !feof($fpi) ) {

								$total_count++;
								$total_one_count++;

								if ( ($rowtext = fgets($fpi)) !== false ) {

									if ( substr($rowtext, 0, 1) !== '#' ) {

										$rowdata = explode('|', $rowtext);

										if ( preg_match('/^[A-Z]{2}$/', $rowdata[1]) && !strcmp($rowdata[2], 'ipv4') ) {	//現在はIPv4のみ

											//国コード
											$country_code = $rowdata[1];

											//IPアドレス / サブネットマスク
											$ip_address = $rowdata[3];
											$subnet_mask = 32 - (int)log($rowdata[4], 2);

											//一覧データへ追記
											$output_text = sprintf("%s\t%s/%s", $country_code, $ip_address, $subnet_mask)."\n";
											if ( fputs($fpo, $output_text) !== false ) {

												//OK!
												$output_count++;
												$output_one_count++;
												
											} else {
												$this->errors[] = '国データ判別ファイルの書き出しに失敗しました';
												break 2;
											}
											
										}
									}

								} else {
									//Skip!
//									$this->errors[] = 'データファイルの読み出しに失敗しました';
//									break 2;
								}
							}
							fclose($fpi);
							
						} else {
							$this->errors[] = 'CIDRファイルの読み出しに失敗しました';
							break;
						}

					}
					fclose($fpo);
					chmod($this->country_data_file, 0666);
				}

			} else {
				$this->errors[] = '国データ判別ファイルの書き出しに失敗しました';
			}

		}

		return $ret;
	}


	/**
	 *	IPアドレスから国を判別する
	 */
	public function DetectCountryCode($ip_addr = null)
	{
		$ret = false;
		$this->errors[] = array();
		
		
		//IPアドレスが指定されていない場合は、$_SERVER を参照する
		if ( !isset($ip_addr) ) {
			if ( isset($_SERVER['REMOTE_ADDR']) ) {
				$ip_addr = $_SERVER['REMOTE_ADDR'];
			}
		}
		
		//国判別データファイルを通読し判定する
		if ( isset($ip_addr) ) {
		
			if ( ($fp = fopen($this->country_data_file, 'r')) !== false ) {
		
				while ( !feof($fp) && ($rowtext = fgets($fp)) !== false ) {
		
					list($country_code, $ip_compare) = explode("\t", $rowtext);
					list($compare_ip, $compare_mask) = explode("/", $ip_compare);
		
					$input_ipmask = ip2long($ip_addr) >> (32 - $compare_mask);
					$check_ipmask = ip2long($compare_ip) >> (32 - $compare_mask);
					
					if ( !strcmp($input_ipmask, $check_ipmask) ) {
					
						$ret = $country_code;
						break;
					}
				}
				
				fclose($fp);
		
			} else {
				$this->errors[] = '国判別データファイルの読み込みに失敗しました';
			}
		}
		
		
		return $ret;
	}
}

