<?php

/*
 * @file: A form for redirecting phonathon-ers to the appropriate form
 * Warning: Only grant users permission to use this when needed (i.e. during the actual phoneathon)
 * NOTE: You must clear the webform results before each phonathon
 */

function woolman_phonathon(&$form_state) {
  if (($result = woolman_aval($_GET, 'result')) && !empty($_GET['cid'])) {
    drupal_set_message('Results of your call: "' . $result . '" with ' . woolman_name('full', $_GET['cid']) . ' (Contact ID ' . $_GET['cid'] . ') have been saved.');
    if ($result == 'Gave credit card by phone') {
      if (!($cs = woolman_aval($_GET, 'cs'))) {
        civicrm_initialize();
        require_once 'CRM/Contact/BAO/Contact/Utils.php';
        $cs = CRM_Contact_BAO_Contact_Utils::generateChecksum($_GET['cid'], NULL, 3);
      }
      drupal_goto('civicrm/contribute/transact',
        array('reset' => 1, 'id' => 1, 'cid' => $_GET['cid'], 'cs' => $cs));
    }
  }

  return array(
    'cid' => array(
      '#prefix' => '<p>Welcome to the Woolman Phonathon!</p>',
      '#type' => 'textfield',
      '#size' => 5,
      '#title' => 'Contact ID',
      '#description' => 'Enter the contact id from your call sheet, leave blank if they don\'t have a sheet.',
    ),
    'call_form' => array(
      '#type' => 'submit',
      '#value' => 'Donor Call Sheet',
    ),
    'cont_form' => array(
      '#type' => 'submit',
      '#value' => 'Credit Card Donation',
    ),
  );
}

function woolman_phonathon_submit($form, &$form_state) {
  $vals = $form_state['values'];
  $q = array();
  if ($cid = $vals['cid']) {
    $q = array('cid' => $vals['cid']);
    civicrm_initialize();
    require_once 'CRM/Contact/BAO/Contact/Utils.php';
    // Generate a short-lived checksum
    $q['cs'] = CRM_Contact_BAO_Contact_Utils::generateChecksum($vals['cid'], NULL, 3);
  }
  switch ($vals['op']) {
    case $vals['call_form']:
      if ($q) {
        if ($s = woolman_phonathon_existing($cid)) {
          drupal_set_message('<strong>Editing your previous call to ' . woolman_name('full', $cid) . ' from ' . format_interval(time() - $s['submitted'], 1) . ' ago.</strong>');
          drupal_set_message(l('Record a new call to ' . woolman_name('full', $cid), 'phone/call', array('query' => $q)) . ' or <a href="/phone">Go back</a> to enter a different contact id.');
          drupal_goto('node/1379/submission/' . $s['sid'] . '/edit');
        }
        drupal_set_message('Please only use this form for ' . woolman_name('full', $cid) . '. <a href="/phone">Go back</a> to enter a different contact id.');
        $rel = array_values(woolman_find_relations($cid, 2));
        if ($rel) {
          $q2 = array('cid' => $rel[0]['contact_id']);
          $q2['cs'] = CRM_Contact_BAO_Contact_Utils::generateChecksum($q2['cid'], NULL, 3);
          drupal_set_message('Switch to ' . l(woolman_name('full', $q2['cid']), 'phone/call', array('query' => $q2)) . ' (spouse)');
        }
      }
      drupal_goto('phone/call', $q);
      break;
    case $vals['cont_form']:
      if ($q) {
        drupal_goto('civicrm/contribute/transact', array('reset' => 1, 'id' => 1) + $q);
      }
      // If no CID, they can't impersonate someone else, we'll just have to log them out.
      drupal_goto('logout', array('destination' => 'civicrm/contribute/transact?reset=1&id=1'));
      break;
  }
}


function woolman_phonathon_existing($cid) {
  global $user;
  $db = db_query("
    SELECT w.sid, w.submitted FROM {webform_submissions} w, {webform_civicrm_submissions} c
    WHERE c.sid = w.sid AND w.uid = %d AND c.contact_id = '-%d-' AND w.nid = 1379
    ORDER BY w.submitted DESC
    LIMIT 0, 1", $user->uid, $cid);
  if ($result = db_fetch_object($db)) {
    return (array) $result;
  }
  return array();
}
