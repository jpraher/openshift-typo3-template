<?php
/***************************************************************
 *  Copyright notice
 *
 *  (c) 1999-2009 Kasper Skårhøj (kasperYYYY@typo3.com)
 *  All rights reserved
 *
 *  This script is part of the TYPO3 project. The TYPO3 project is
 *  free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  The GNU General Public License can be found at
 *  http://www.gnu.org/copyleft/gpl.html.
 *  A copy is found in the textfile GPL.txt and important notices to the license
 *  from the author is found in LICENSE.txt distributed with these scripts.
 *
 *
 *  This script is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  This copyright notice MUST APPEAR in all copies of the script!
 ***************************************************************/
/**
 * Generating plain text content of content elements for Direct Mails
 *
 * Alternative rendering of content elements for Plain Text emails. That means text-only output. No HTML at all. Used by the Direct Mail extension.
 * Normally the plain text output should appear with type=99.
 * To use this library you can include the static template "plugin.alt.plaintext"
 *
 * ## Insert DMailer Boundaries for all elements.
 * config.insertDmailerBoundaries = 1
 * includeLibs.plaintextLib = media/scripts/plaintextLib.inc
 *
 * ## Set up page/type number:
 * alt_plaintext >
 * alt_plaintext = PAGE
 * alt_plaintext.typeNum=99
 * alt_plaintext.config.disableAllHeaderCode = 1
 * alt_plaintext.10 = TEMPLATE
 * alt_plaintext.10 {
 * template = FILE
 * template.file = {$plugin.alt.plaintext.file.template}
 * marks.CONTENT < styles.content.get
 * marks.CONTENT.renderObj = < lib.alt_plaintext.renderObj
 * marks.DATE = TEXT
 * marks.DATE.data = date:U
 * marks.DATE.strftime = %e. %B %Y
 * }
 *
 * (And then also "lib.alt_plaintext.renderObj" is configured extensively - basically with the TypoScript options passed to this class. See the static template "plugin.alt.plaintext")
 *
 * @author Kasper Skårhøj <kasperYYYY@typo3.com>
 */
class user_plaintext {

	/**
	 * @todo Define visibility
	 */
	public $cObj;

	/**
	 * @todo Define visibility
	 */
	public $conf = array();

	/**
	 * @todo Define visibility
	 */
	public $charWidth = 76;

	/**
	 * Main function, called from TypoScript
	 * A content object that renders "tt_content" records. See the comment to this class for TypoScript example of how to trigger it.
	 * This detects the CType of the current content element and renders it accordingly. Only wellknown types are rendered.
	 *
	 * @param string $content Empty, ignore.
	 * @param array $conf TypoScript properties for this content object/function call
	 * @return string Plain text content
	 * @todo Define visibility
	 */
	public function main_plaintext($content, $conf) {
		$this->conf = $conf;
		$this->siteUrl = $conf['siteUrl'];
		$lines = array();
		$CType = (string) $this->cObj->data['CType'];
		switch ($CType) {
		case 'header':
			$lines[] = $this->getHeader();
			if ($this->cObj->data['subheader']) {
				$lines[] = $this->breakContent(strip_tags($this->cObj->data['subheader']));
			}
			break;
		case 'text':

		case 'textpic':
			$lines[] = $this->getHeader();
			if ($CType == 'textpic' && !($this->cObj->data['imageorient'] & 24)) {
				$lines[] = $this->getImages();
				$lines[] = '';
			}
			$lines[] = $this->breakContent(strip_tags($this->parseBody($this->cObj->data['bodytext'])));
			if ($CType == 'textpic' && $this->cObj->data['imageorient'] & 24) {
				$lines[] = '';
				$lines[] = $this->getImages();
			}
			break;
		case 'image':
			$lines[] = $this->getHeader();
			$lines[] = $this->getImages();
			break;
		case 'uploads':
			$lines[] = $this->getHeader();
			$lines[] = $this->renderUploads($this->cObj->data['media']);
			break;
		case 'menu':
			$lines[] = $this->getHeader();
			$lines[] = $this->getMenuSitemap();
			break;
		case 'shortcut':
			$lines[] = $this->getShortcut();
			break;
		case 'bullets':
			$lines[] = $this->getHeader();
			$lines[] = $this->breakBulletlist(strip_tags($this->parseBody($this->cObj->data['bodytext'])));
			break;
		case 'list':
			$lines[] = $this->getHeader();
			$lines[] = $this->getList();
			break;
		case 'table':
			$lines[] = $this->getHeader();
			$lines[] = $this->breakTable(strip_tags($this->parseBody($this->cObj->data['bodytext'])));
			break;
		case 'html':
			$lines[] = $this->getHTML();
			break;
		default:
			$defaultOutput = $this->getString($this->conf['defaultOutput']);
			if ($defaultOutput) {
				$lines[] = str_replace('###CType###', $CType, $defaultOutput);
			}
			break;
		}
		// First break.
		$lines[] = '';
		$content = implode(chr(10), $lines);
		// User processing:
		$content = $this->userProcess('userProc', $content);
		return $content;
	}

	/**
	 * Creates a menu/sitemap
	 *
	 * @return string Content
	 * @todo Define visibility
	 */
	public function getMenuSitemap() {
		$str = $this->cObj->cObjGetSingle($this->conf['menu'], $this->conf['menu.']);
		$str = $this->breakBulletlist(trim(strip_tags(preg_replace('/<br[ \\/]*>/i', chr(10), $this->parseBody($str)))));
		return $str;
	}

	/**
	 * Creates a shortcut ("Insert Records")
	 *
	 * @return string Content
	 * @todo Define visibility
	 */
	public function getShortcut() {
		$str = $this->cObj->cObjGetSingle($this->conf['shortcut'], $this->conf['shortcut.']);
		return $str;
	}

	/**
	 * Creates an HTML element (stripping tags of course)
	 *
	 * @param string $str HTML content to process. If not passed along, the bodytext field is used.
	 * @return string Content
	 * @todo Define visibility
	 */
	public function getHTML($str = array()) {
		return $this->breakContent(strip_tags(preg_replace('/<br[ \\/]*>/i', chr(10), $this->parseBody(is_string($str) ? $str : $this->cObj->data['bodytext']))));
	}

	/**
	 * Creates a header (used for most elements)
	 *
	 * @return string Content
	 * @see renderHeader()
	 * @todo Define visibility
	 */
	public function getHeader() {
		// links...
		return $this->renderHeader($this->cObj->data['header'], $this->cObj->data['header_layout']);
	}

	/**
	 * Get images found in the "image" field of "tt_content"
	 *
	 * @return string Content
	 * @todo Define visibility
	 */
	public function getImages() {
		$images = $this->renderImages($this->cObj->data['image'], !$this->cObj->data['image_zoom'] ? $this->cObj->data['image_link'] : '', $this->cObj->data['imagecaption']);
		return $images;
	}

	/**
	 * Parsing the bodytext field content, removing typical entities and <br /> tags.
	 *
	 * @param string $str Field content from "bodytext
	 * @return string Processed content
	 * @todo Define visibility
	 */
	public function parseBody($str) {
		// First, regular parsing:
		$str = preg_replace('/<br[ \\/]*>/i', ' ', $str);
		$str = $this->cObj->stdWrap($str, $this->conf['bodytext.']['stdWrap.']);
		// Then all a-tags:
		$aConf = array();
		$aConf['parseFunc.']['tags.']['a'] = 'USER';
		$aConf['parseFunc.']['tags.']['a.']['userFunc'] = 'user_plaintext->atag_to_http';
		$aConf['parseFunc.']['tags.']['a.']['siteUrl'] = $this->siteUrl;
		$str = $this->cObj->stdWrap($str, $aConf);
		$str = str_replace('&nbsp;', ' ', t3lib_div::htmlspecialchars_decode($str));
		return $str;
	}

	/**
	 * Creates a list of links to uploaded files.
	 *
	 * @param string $str List of uploaded filenames from "uploads/media/" (or $upload_path)
	 * @param string $upload_path Alternative path value
	 * @return string Content
	 * @todo Define visibility
	 */
	public function renderUploads($str, $upload_path = 'uploads/media/') {
		$files = explode(',', $str);
		$lines = array();
		if ($this->conf['uploads.']['header']) {
			$lines[] = $this->getString($this->conf['uploads.']['header']);
		}
		foreach ($files as $k => $file) {
			$lines[] = ($this->siteUrl . $upload_path) . $file;
		}
		return implode(chr(10), $lines);
	}

	/**
	 * Creates a list
	 *
	 * @param string $CType Type of content
	 * @return string Content
	 * @todo Define visibility
	 */
	public function getList($CType = 'list') {
		$str = $this->cObj->cObjGetSingle($this->conf[$CType], $this->conf[$CType . '.']);
		return trim(strip_tags($this->parseBody($str)));
	}

	/**
	 * Renders a content element header, observing the layout type giving different header formattings
	 *
	 * @param string $str The header string
	 * @param integer $type The layout type of the header (in the content element)
	 * @return string Content
	 * @todo Define visibility
	 */
	public function renderHeader($str, $type = 0) {
		if ($str) {
			$hConf = $this->conf['header.'];
			$defaultType = t3lib_utility_Math::forceIntegerInRange($hConf['defaultType'], 1, 5);
			$type = t3lib_utility_Math::forceIntegerInRange($type, 0, 6);
			if (!$type) {
				$type = $defaultType;
			}
			if ($type != 6) {
				// not hidden
				$tConf = $hConf[$type . '.'];
				$lines = array();
				$blanks = t3lib_utility_Math::forceIntegerInRange($tConf['preBlanks'], 0, 1000);
				if ($blanks) {
					$lines[] = str_pad('', $blanks - 1, chr(10));
				}
				$lines = $this->pad($lines, $tConf['preLineChar'], $tConf['preLineLen']);
				$blanks = t3lib_utility_Math::forceIntegerInRange($tConf['preLineBlanks'], 0, 1000);
				if ($blanks) {
					$lines[] = str_pad('', $blanks - 1, chr(10));
				}
				if ($this->cObj->data['date']) {
					$lines[] = $this->getString($hConf['datePrefix']) . date(($hConf['date'] ? $hConf['date'] : $GLOBALS['TYPO3_CONF_VARS']['SYS']['ddmmyy']), $this->cObj->data['date']);
				}
				$prefix = '';
				$str = $this->getString($tConf['prefix']) . $str;
				if ($tConf['autonumber']) {
					$str = $this->cObj->parentRecordNumber . $str;
				}
				if ($this->cObj->data['header_position'] == 'right') {
					$prefix = str_pad(' ', $this->charWidth - strlen($str));
				}
				if ($this->cObj->data['header_position'] == 'center') {
					$prefix = str_pad(' ', floor(($this->charWidth - strlen($str)) / 2));
				}
				$lines[] = $this->cObj->stdWrap($prefix . $str, $tConf['stdWrap.']);
				if ($this->cObj->data['header_link']) {
					$lines[] = $this->getString($hConf['linkPrefix']) . $this->getLink($this->cObj->data['header_link']);
				}
				$blanks = t3lib_utility_Math::forceIntegerInRange($tConf['postLineBlanks'], 0, 1000);
				if ($blanks) {
					$lines[] = str_pad('', $blanks - 1, chr(10));
				}
				$lines = $this->pad($lines, $tConf['postLineChar'], $tConf['postLineLen']);
				$blanks = t3lib_utility_Math::forceIntegerInRange($tConf['postBlanks'], 0, 1000);
				if ($blanks) {
					$lines[] = str_pad('', $blanks - 1, chr(10));
				}
				return implode(chr(10), $lines);
			}
		}
	}

	/**
	 * Function used to repeat a char pattern in head lines (like if you want "********" above/below a header)
	 *
	 * @param array $lines Array of existing lines to which the new char-pattern should be added
	 * @param string $preLineChar The character pattern to repeat. Default is "-
	 * @param integer $len The length of the line. $preLineChar will be repeated to fill in this length.
	 * @return array The input array with a new line added.
	 * @see renderHeader()
	 * @todo Define visibility
	 */
	public function pad($lines, $preLineChar, $len) {
		$strPad = t3lib_utility_Math::forceIntegerInRange($len, 0, 1000);
		$strPadChar = $preLineChar ? $preLineChar : '-';
		if ($strPad) {
			$lines[] = str_pad('', $strPad, $strPadChar);
		}
		return $lines;
	}

	/**
	 * Function used to wrap the bodytext field content (or image caption) into lines of a max length of
	 *
	 * @param string The content to break
	 * @return string Processed value.
	 * @see main_plaintext(), breakLines()
	 * @todo Define visibility
	 */
	public function breakContent($str) {
		$cParts = explode(chr(10), $str);
		$lines = array();
		foreach ($cParts as $substrs) {
			$lines[] = $this->breakLines($substrs);
		}
		return implode(chr(10), $lines);
	}

	/**
	 * Breaks content lines into a bullet list
	 *
	 * @param string $str Content string to make into a bullet list
	 * @return string Processed value
	 * @todo Define visibility
	 */
	public function breakBulletlist($str) {
		$type = $this->cObj->data['layout'];
		$type = t3lib_utility_Math::forceIntegerInRange($type, 0, 3);
		$tConf = $this->conf['bulletlist.'][$type . '.'];
		$cParts = explode(chr(10), $str);
		$lines = array();
		$c = 0;
		foreach ($cParts as $substrs) {
			$c++;
			$bullet = $tConf['bullet'] ? $this->getString($tConf['bullet']) : ' - ';
			$bLen = strlen($bullet);
			$bullet = substr(str_replace('#', $c, $bullet), 0, $bLen);
			$secondRow = substr($tConf['secondRow'] ? $this->getString($tConf['secondRow']) : str_pad('', strlen($bullet), ' '), 0, $bLen);
			$lines[] = $bullet . $this->breakLines($substrs, (chr(10) . $secondRow), ($this->charWidth - $bLen));
			$blanks = t3lib_utility_Math::forceIntegerInRange($tConf['blanks'], 0, 1000);
			if ($blanks) {
				$lines[] = str_pad('', $blanks - 1, chr(10));
			}
		}
		return implode(chr(10), $lines);
	}

	/**
	 * Formatting a table in plain text (based on the paradigm of lines being content rows and cells separated by "|")
	 *
	 * @param string $str Content string
	 * @return string Processed value
	 * @todo Define visibility
	 */
	public function breakTable($str) {
		$cParts = explode(chr(10), $str);
		$lines = array();
		$cols = intval($this->conf['cols']) ? intval($this->conf['cols']) : 0;
		$c = 0;
		foreach ($cParts as $substrs) {
			$c++;
			if (trim($substrs)) {
				$lineParts = explode('|', $substrs);
				if (!$cols) {
					$cols = count($lineParts);
				}
				for ($a = 0; $a < $cols; $a++) {
					$jdu = explode(chr(10), $this->breakLines($lineParts[$a], chr(10), ceil($this->charWidth / $cols)));
					$lines[$c][$a] = $jdu;
				}
			}
		}
		$messure = $this->traverseTable($lines);
		$divChar = '-';
		$joinChar = '+';
		$colChar = '|';
		// Make table:
		$outLines = array();
		$outLines[] = $this->addDiv($messure, '', $divChar, $joinChar, $cols);
		foreach ($lines as $k => $v) {
			$top = intval($messure[1][$k]);
			for ($aa = 0; $aa < $top; $aa++) {
				$tempArr = array();
				for ($bb = 0; $bb < $cols; $bb++) {
					$tempArr[$bb] = str_pad($v[$bb][$aa], $messure[0][$bb], ' ');
				}
				$outLines[] = ($colChar . implode($colChar, $tempArr)) . $colChar;
			}
			$outLines[] = $this->addDiv($messure, '', $divChar, $joinChar, $cols);
		}
		return implode(chr(10), $outLines);
	}

	/**
	 * Subfunction for breakTable(): Adds a divider line between table rows.
	 *
	 * @param array $messure Some information about sizes
	 * @param string $content Empty string.
	 * @param string $divChar Character to use for the divider line, typically "-
	 * @param string $joinChar Join character, typically "+
	 * @param integer $colsNumber of table columns
	 * @return string Divider line for the table
	 * @access private
	 * @see breakTable()
	 * @todo Define visibility
	 */
	public function addDiv($messure, $content, $divChar, $joinChar, $cols) {
		$tempArr = array();
		for ($a = 0; $a < $cols; $a++) {
			$tempArr[$a] = str_pad($content, $messure[0][$a], $divChar);
		}
		return ($joinChar . implode($joinChar, $tempArr)) . $joinChar;
	}

	/**
	 * Traverses the table lines/cells and creates arrays with statistics for line numbers and lengths
	 *
	 * @param array $tableLines Array with [table rows] [table cells] [lines in cell]
	 * @return array Statistics (max lines/lengths)
	 * @access private
	 * @see breakTable()
	 * @todo Define visibility
	 */
	public function traverseTable($tableLines) {
		$maxLen = array();
		$maxLines = array();
		foreach ($tableLines as $k => $v) {
			foreach ($v as $kk => $vv) {
				foreach ($vv as $lk => $lv) {
					if (strlen($lv) > intval($maxLen[$kk])) {
						$maxLen[$kk] = strlen($lv);
					}
				}
				if (count($vv) > intval($maxLines[$k])) {
					$maxLines[$k] = count($vv);
				}
			}
		}
		return array($maxLen, $maxLines);
	}

	/**
	 * Render block of images - which means creating lines with links to the images.
	 *
	 * @param string $str List of image filenames (from "image" field in tt_content records)
	 * @param string $links Link value from the "image_link" field in tt_content records
	 * @param string $caption Caption text
	 * @param string $upload_path Alternative relative path for the files listed in $str
	 * @return string Content
	 * @see getImages()
	 * @todo Define visibility
	 */
	public function renderImages($str, $links, $caption, $upload_path = 'uploads/pics/') {
		$images = explode(',', $str);
		$linksArr = explode(LF, $links);
		$lines = array();
		if ($this->conf['images.']['header']) {
			$lines[] = $this->getString($this->conf['images.']['header']);
		}
		while (list($k, $file) = each($images)) {
			$lines[] = ($this->siteUrl . $upload_path) . $file;
			if ($links && count($linksArr) > 1) {
				if (isset($linksArr[$k])) {
					$ll = $linksArr[$k];
				} else {
					$ll = $linksArr[0];
				}
				$theLink = $this->getLink($ll);
				if ($theLink) {
					$lines[] = $this->getString($this->conf['images.']['linkPrefix']) . $theLink;
				}
			}
		}
		if ($links && count($linksArr) == 1) {
			$theLink = $this->getLink($links);
			if ($theLink) {
				$lines[] = $this->getString($this->conf['images.']['linkPrefix']) . $theLink;
			}
		}
		if ($caption) {
			$lines[] = '';
			$cHeader = trim($this->getString($this->conf['images.']['captionHeader']));
			if ($cHeader) {
				$lines[] = $cHeader;
			}
			$lines[] = $this->breakContent($caption);
		}
		return implode(chr(10), $lines);
	}

	/**
	 * Returns a typolink URL based on input.
	 *
	 * @param string $ll Parameter to typolink
	 * @return string The URL returned from $this->cObj->getTypoLink_URL(); - possibly it prefixed with the URL of the site if not present already
	 * @todo Define visibility
	 */
	public function getLink($ll) {
		$theLink = $this->cObj->getTypoLink_URL($ll);
		if (substr($theLink, 0, 4) != 'http') {
			$theLink = $this->siteUrl . $theLink;
		}
		return $theLink;
	}

	/**
	 * Breaking lines into fixed length lines, using t3lib_div::breakLinesForEmail()
	 *
	 * @param string $str The string to break
	 * @param string $implChar Line break character
	 * @param integer $charWidth Length of lines, default is $this->charWidth
	 * @return string Processed string
	 * @see t3lib_div::breakLinesForEmail()
	 * @todo Define visibility
	 */
	public function breakLines($str, $implChar = '
', $charWidth = 0) {
		return t3lib_div::breakLinesForEmail($str, $implChar, $charWidth ? $charWidth : $this->charWidth);
	}

	/**
	 * Explodes a string with "|" and if the second part is found it will return this, otherwise the first part.
	 * Used for many TypoScript properties used in this class since they need preceeding whitespace to be preserved.
	 *
	 * @param string $str Input string
	 * @return string Output string
	 * @access private
	 * @todo Define visibility
	 */
	public function getString($str) {
		$parts = explode('|', $str);
		return strcmp($parts[1], '') ? $parts[1] : $parts[0];
	}

	/**
	 * Calls a user function for processing of data
	 *
	 * @param string $mConfKey TypoScript property name, pointing to the definition of the user function to call (from the TypoScript array internally in this class). This array is passed to the user function. Notice that "parentObj" property is a reference to this class ($this)
	 * @param mixed $passVar Variable to process
	 * @return mixed The processed $passVar as returned by the function call
	 * @todo Define visibility
	 */
	public function userProcess($mConfKey, $passVar) {
		if ($this->conf[$mConfKey]) {
			$funcConf = $this->conf[$mConfKey . '.'];
			$funcConf['parentObj'] = $this;
			$passVar = $GLOBALS['TSFE']->cObj->callUserFunction($this->conf[$mConfKey], $funcConf, $passVar);
		}
		return $passVar;
	}

	/**
	 * Function used by TypoScript "parseFunc" to process links in the bodytext.
	 * Extracts the link and shows it in plain text in a parathesis next to the link text. If link was relative the site URL was prepended.
	 *
	 * @param string $content Empty, ignore.
	 * @param array $conf TypoScript parameters
	 * @return string Processed output.
	 * @see parseBody()
	 * @todo Define visibility
	 */
	public function atag_to_http($content, $conf) {
		$this->conf = $conf;
		$this->siteUrl = $conf['siteUrl'];
		$theLink = trim($this->cObj->parameters['href']);
		if (strtolower(substr($theLink, 0, 7)) == 'mailto:') {
			$theLink = substr($theLink, 7);
		} elseif (substr($theLink, 0, 4) != 'http') {
			$theLink = $this->siteUrl . $theLink;
		}
		return (($this->cObj->getCurrentVal() . ' (Link: ') . $theLink) . ' )';
	}

	/**
	 * User function (called from TypoScript) for generating a bullet list (used in parsefunc)
	 *
	 * @param string $content Empty, ignore.
	 * @param array $conf TypoScript parameters
	 * @return string Processed output.
	 * @todo Define visibility
	 */
	public function typolist($content, $conf) {
		$this->conf = $this->cObj->mergeTSRef($conf, 'bulletlist');
		$this->siteUrl = $conf['siteUrl'];
		$str = trim($this->cObj->getCurrentVal());
		$this->cObj->data['layout'] = $this->cObj->parameters['type'];
		return $this->breakBulletlist($str);
	}

	/**
	 * User function (called from TypoScript) for generating a typo header tag (used in parsefunc)
	 *
	 * @param string $content Empty, ignore.
	 * @param array $conf TypoScript parameters
	 * @return string Processed output.
	 * @todo Define visibility
	 */
	public function typohead($content, $conf) {
		$this->conf = $this->cObj->mergeTSRef($conf, 'header');
		$this->siteUrl = $conf['siteUrl'];
		$str = trim($this->cObj->getCurrentVal());
		$this->cObj->data['header_layout'] = $this->cObj->parameters['type'];
		$this->cObj->data['header_position'] = $this->cObj->parameters['align'];
		$this->cObj->data['header'] = $str;
		return $this->getHeader();
	}

	/**
	 * User function (called from TypoScript) for generating a code listing (used in parsefunc)
	 *
	 * @param string $content Empty, ignore.
	 * @param array $conf TypoScript parameters
	 * @return string Processed output.
	 * @todo Define visibility
	 */
	public function typocode($content, $conf) {
		// Nothing is really done here...
		$this->conf = $conf;
		$this->siteUrl = $conf['siteUrl'];
		return $this->cObj->getCurrentVal();
	}

}

?>