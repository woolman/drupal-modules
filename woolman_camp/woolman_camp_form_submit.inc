<?php

module_load_include('inc', 'woolman_camp', 'woolman_camp_utils');

/**
 * First Time Form Submit
 */
function woolman_camp_first_time_form_submit($form, &$form_state) {
  $vals = $form_state['values'];
  // Record how parent heard about us
  woolman_civicrm_api('activity', 'create', array(
    'activity_type_id' => 53,
    'source_contact_id' => woolman_user_cid(),
    'target_contact_id' => woolman_user_cid(),
    'subject' => $form['self_medium']['#options'][$vals['self_medium']],
    'status_id' => 2,
    'activity_date_time' => date('Ymdhis'),
    'details' => '<p><em>Entered on the Camps Woolman registration form:</em></p><p>' . $vals['self_details'] . '</p>',
    'custom_6' => woolman_multivalue_str('summer camp'),
    'custom_7' => $vals['self_medium'],
  ));
}

/**
 * Navigation Form submit
 * Updates registration activity to allow user to move back to a previous step in the process
 */
function woolman_camp_nav_form_submit($form, &$form_state) {
  switch ($form_state['clicked_button']['#id']) {
    case 'edit-button-1':
      $status = 6;
    break;
    case 'edit-button-2':
      $status = 4;
    break;
  }
  woolman_camp_update_reg_act($status);
}

/**
 * Family Info Form Validate
 */
function woolman_camp_family_form_validate($form, &$form_state) {
  $vals = $form_state['values'];
  $required = array('first', 'last', 'dob', 'gender');
  $address = array('street', 'city', 'state', 'zip', 'country');

  foreach (array('self' => array('your','yourself'), 'spouse' => array('your spouse\'s','your spouse'), 'e_contact' => array('your emergency contact\'s','your emergency contact')) as $p => $w) {
    $skip = 0;
    if (!empty($vals[$p . '_na'])) {
      continue;
    }
    if ($p=='spouse' && $vals['not_spouse_checkbox'] && !$vals['not_spouse_select']) {
      form_set_error('not_spouse_select', 'Please choose an option regarding your previous spouse/partner.');
    }
    foreach ($required as $req) {
      if ((empty($vals[$p . '_' . $req]) && !($req == 'dob' && $p == 'e_contact'))
      || ($req == 'dob' && woolman_aval($vals, $p . '_dob:year') == 1900)) {
        if ($p == 'spouse' && $req == 'first') {
          form_set_error('spouse_na', 'Please fill information about your spouse, or else check the "Not Applicable" button.');
          break;$skip = 1;
        }
        else
          form_set_error($p . '_' . $req, 'Please fill in ' . $w[0] . ' '.strtolower($form[$p . '_wrapper'][$p][$p . '_' . $req]['#title']) . '.');
      }
    }
    if ($skip) {
      continue;
    }
    if (!$vals[$p . '_phone_home'] && !$vals[$p . '_phone_cell'] && !$vals[$p . '_phone_work']) {
      form_set_error($p . '_phone_' . $vals[$p . '_phone_primary'], 'Please fill in at least one phone number for ' . $w[1] . '.');
    }
    elseif (!$vals[$p . '_phone_' . $vals[$p . '_phone_primary']]) {
      form_set_error($p . '_phone_primary', 'You indicated that ' . $w[0] . ' primary phone number is "' . $vals[$p . '_phone_primary'] . '" but didn\'t provide a ' . $vals[$p . '_phone_primary'] . ' phone number.');
      form_set_error($p . '_phone_' . $vals[$p . '_phone_primary'], 'Please fill in the ' . $vals[$p . '_phone_primary'] . ' number for ' . $w[1] . ' or else choose a different number as the primary one.');
    }
    if (!$vals[$p . '_email_home'] && !$vals[$p . '_email_work']) {
      form_set_error($p . '_email_' . $vals[$p . '_email_primary'], 'Please fill in at least one email address for ' . $w[1] . '.');
    }
    elseif (!$vals[$p . '_email_' . $vals[$p . '_email_primary']]) {
      form_set_error($p . '_email_primary', 'You indicated that ' . $w[0] . ' primary email is "' . $vals[$p . '_email_primary'] . '" but didn\'t provide a ' . $vals[$p . '_email_primary'] . ' email address.');
      form_set_error($p . '_email_' . $vals[$p . '_email_primary'], 'Please fill in the ' . $vals[$p . '_email_primary'] . ' email for ' . $w[1] . ' or else choose a different address as the primary one.');
    }
    if (!$vals[$p . '_same']) {
      foreach ($address as $req) {
        if (!$vals[$p . '_' . $req]) {
          form_set_error($p . '_' . $req, 'Please fill in ' . $w[0] . ' '.strtolower($form[$p . '_wrapper'][$p][$p . '_address'][$p . '_' . $req]['#title']) . '.');
        }
      }
    }
  }
  // Now the kids
  for ($c = 1; $c <= $vals['number_of_children']; $c++) {
    foreach ($required as $req) {
      if (!$vals['child' . $c . '_' . $req]) {
        form_set_error('child' . $c . '_' . $req, 'Please fill in child ' . $c . '\'s ' . strtolower($form['children_wrapper']['child' . $c]['child' . $c . '_' . $req]['#title']) . '.');
      }
    }
  }
}

/**
 * Family Info Form Submit
 */
function woolman_camp_family_form_submit($form, &$form_state) {
  $vals = $form_state['values'];
  $vals['self_cid'] = woolman_user_cid();
  $exclude = '';

  // Add or update contacts
  foreach (array('self', 'spouse', 'e_contact') as $p) {
    if (!empty($vals[$p . '_na'])) {
      continue;
    }
    $params = array(
      'first_name' => $vals[$p . '_first'],
      'nick_name' => $vals[$p . '_nick'],
      'middle_name' => $vals[$p . '_middle'],
      'last_name' => $vals[$p . '_last'],
      'gender_id' => $vals[$p . '_gender'],
    );
    if (!empty($vals[$p . '_dob'])) {
      $params['birth_date'] = woolman_date_array_to_string($vals[$p . '_dob']);
    }

    if ($vals[$p . '_cid'] && !$vals['not_' . $p . '_checkbox']) {
      $params['contact_id'] = $vals[$p . '_cid'];
    }

    $address = array(array(
      'location_type_id' => 1,
      'is_primary' => 1,
      'street_address' => $vals[$p . '_street'],
      'city' => $vals[$p . '_city'],
      'state_province' => $vals[$p . '_state'],
      'postal_code' => $vals[$p . '_zip'],
      'country_id' => $vals[$p . '_country'],
    ));

    $phone = array();
    if ($vals[$p . '_phone_cell']) {
      $phone[] = array(
        'phone' => $vals[$p . '_phone_cell'],
        'location_type_id' => 4,
        'is_primary' => ($vals[$p . '_phone_primary']=='cell'),
      );
    }
    if ($vals[$p . '_phone_home']) {
      $phone[] = array(
        'phone' => $vals[$p . '_phone_home'],
        'location_type_id' => 1,
        'is_primary' => ($vals[$p . '_phone_primary']=='home'),
      );
    }
    if ($vals[$p . '_phone_work']) {
      $phone[] = array(
        'phone' => $vals[$p . '_phone_work'],
        'location_type_id' => 2,
        'is_primary' => ($vals[$p . '_phone_primary']=='work'),
      );
    }
    $email = array();
    if ($vals[$p . '_email_home']) {
      $email[] = array('email' => $vals[$p . '_email_home'],
                        'location_type_id' => 1,
                        'is_primary' => ($vals[$p . '_email_primary']=='home'),
                        );
      $adult_emails[strtolower($vals[$p . '_email_home'])] = TRUE;
    }
    if ($vals[$p . '_email_work'] && $vals[$p . '_email_work'] != $vals[$p . '_email_home'])  {
      $email[] = array(
        'email' => $vals[$p . '_email_work'],
        'location_type_id' => 2,
        'is_primary' => ($vals[$p . '_email_primary']=='work'),
      );
      $adult_emails[strtolower($vals[$p . '_email_work'])] = TRUE;
    }

    $result = woolman_contact_match(
      array(
        'contact' => $params,
        'address' => ($vals[$p . '_same'] ? array('share_with_contact' => $cid['self']) : $address),
        'phone' => $phone,
        'email' => $email
      ),
      'update_all', 'return_cid', 'strict'
    );
    $cid[$p] = $result['contact_id'];
    $exclude .= ($exclude ? ',' : '') . $cid[$p];
  }
  // Add/update kids
  $kid_params = array('address' => array('share_with_contact' => $cid['self']));
  if ($vals['self_phone_home']) {
    $kid_params['phone'] = array(array(
      'phone' => $vals['self_phone_home'],
      'phone_type_id' => 1,
      'location_type_id' => 1,
      'is_primary' => 1,
    ));
  }
  $kids = '';
  for ($c = 1; $c <= $vals['number_of_children']; $c++) {
    $kid_params['contact'] = array(
      'first_name' => $vals['child' . $c . '_first'],
      'nick_name' => $vals['child' . $c . '_nick'],
      'middle_name' => $vals['child' . $c . '_middle'],
      'last_name' => $vals['child' . $c . '_last'],
      'gender_id' => $vals['child' . $c . '_gender'],
      'birth_date' => woolman_date_array_to_string($vals['child' . $c . '_dob']),
      'contact_id' => woolman_aval($vals, 'child' . $c . '_cid'),
    );

    unset($kid_params['email']);
    if ($vals['child' . $c . '_email'] && !($adult_emails[strtolower($vals['child' . $c . '_email'])])) {
      $kid_params['email'][] = array(
        'email' => $vals['child' . $c . '_email'],
        'location_type_id' => 1,
        'is_primary' => 1,
      );
    }
    $result = woolman_contact_match($kid_params, 'update_all', 'return_cid', 'strict', $exclude);
    $cid['child' . $c] = $result['contact_id'];
    $exclude .= ',' . $result['contact_id'];
    $kids .= ($kids ? ',' : '') . $cid['child' . $c];

    // This kid's relationships
    $params = array(
      'contact_id_a' => $cid['child' . $c],
      'contact_id_b' => $cid['self'],
      'relationship_type_id' => 1,
      'custom_8' => $vals['child' . $c . '_relationship'],
      'is_active' => 1
    );
    woolman_relationship_add_or_update($params, TRUE);
    if ($params['contact_id_b'] = $cid['spouse']) {
      $params['custom_8'] = $vals['child' . $c . '_relationship_spouse'];
      woolman_relationship_add_or_update($params, TRUE);
    }
    // E-contact relationship
    // we do it twice so start date is only entered for new relationships,
    // but old inactive ones get updated to be active again
    if (empty($vals['e_contact_na'])) {
      unset($params['custom_8']);
      $params['contact_id_b'] = $cid['e_contact'];
      $params['relationship_type_id'] = 19;
      $params['start_date'] = date('Ymd');
      woolman_relationship_add_or_update($params);
      unset($params['start_date']);
      woolman_relationship_add_or_update($params, TRUE);
    }
  }

  // Spouse relationship
  $params = array(
    'contact_id_a' => $cid['self'],
    'contact_id_b' => $cid['spouse'],
    'relationship_type_id' => 2,
    'custom_9' => $vals['spouse_relationship'],
    'is_active' => (bool) $vals['spouse_relationship']
  );
  if (empty($vals['spouse_na'])) {
    woolman_relationship_add_or_update($params, TRUE);
  }

  // End old spouse relationship
  if ($vals['not_spouse_checkbox']) {
    $result = woolman_civicrm_api('relationship', 'get', array('contact_id' => $cid['self']));
    $i = 0;
    if (is_array($result['values'])) {
      foreach ($result['values'] as $rid => $rel) {
        if (($rel['contact_id_a'] != $vals['spouse_cid'] && $rel['contact_id_b'] != $vals['spouse_cid']) || $rel['relation'] != 'Spouse of') {
          continue;
        }
        $params = array('id' => $rid, 'relationship_type_id' => 2);
        if ($vals['not_spouse_select'] == 2 || (++$i)> 1) {
          woolman_civicrm_api('relationship', 'delete', $params);
        }
        else {
          $params['is_active'] = 0;
          $params['contact_id_a'] = $rel['contact_id_a'];
          $params['contact_id_b'] = $rel['contact_id_b'];
          woolman_civicrm_api('relationship', 'create', $params);
        }
      }
    }
  }
  // End old e-contact relationships
  if ($vals['not_e_contact_checkbox']) {
    for ($c = 1; $kid = $vals['child' . $c . '_cid']; $c++) {
      if (strpos($kids, $kid) === FALSE) {
        $kids .= ',' . $kid;
      }
    }
    db_query("UPDATE {civicrm_relationship} SET is_active = 0, end_date = CURDATE()
              WHERE contact_id_a IN ($kids) AND contact_id_b = %d AND is_active = 1 AND relationship_type_id =
                (SELECT id FROM {civicrm_relationship_type} WHERE name_a_b = 'Emergency Contact is')", $vals['e_contact_cid']);
  }

  // Add to camp mailing list
  $params = array(
    'contact_id' => $cid['self'],
    'group_id' => 12
  );
  woolman_civicrm_api('group_contact', 'create', $params);
  if ($cid['spouse']) {
    $params['contact_id'] = $cid['spouse'];
    woolman_civicrm_api('group_contact', 'create', $params);
  }

  // Add or update registration activity
  woolman_camp_update_reg_act(4);
}

/**
 * Register Form Validate
 */
function woolman_camp_registration_form_validate($form, &$form_state) {
  $vals = $form_state['values'];
  $ok = TRUE;
  $total = 0;
  foreach ($vals as $id => $val) {
    if (substr($id, 0, 5) != 'child' || empty($val['questions'])) {
      continue;
    }
    foreach (woolman_camp_event_types() as $camp => $program) {
      if (array_sum(woolman_aval($val, "register_$camp", array())) > 0) {
        $total++;
        foreach ($val['questions'] as $k => $q) {
          if (!$q && strpos($k, 'parent') !== FALSE) {
            $ok = FALSE;
            form_error($form[$id]['questions'][$k], 'Please answer parent registration questions.');
            break;
          }
        }
        break;
      }
    }
  }
  if (!$total) {
    // If no boxes have been checked, see if they have registered at all.
    // If so, they may be un-registering, which is allowed.
    // If not, then return false.
    $year = woolman_camp_next_year();
    list($children, $spouse, $e_contact, $prior_reg) = woolman_camp_fetch_relations();
    $registered = FALSE;
    foreach ($children as $child) {
      if ($child['prior_camp']) {
        foreach ($child['prior_camp'] as $d => $e) {
          $y = substr($d, 0, 4);
          if ($y == $year) {
            $registered = TRUE;
          }
        }
      }
    }
    if (!$registered) {
      $ok = FALSE;
      drupal_set_message('You haven\'t registered any campers yet.', 'error');
    }
  }
  return $ok;
}

/**
 * Register Form Submit
 */
function woolman_camp_registration_form_submit($form, &$form_state) {
  module_load_include('inc', 'woolman_website', 'woolman_website_utils');
  $vals = $form_state['values'];
  $sessions = woolman_camp_get_sessions(woolman_camp_next_year());
  $parent = woolman_user_cid();
  $rates = woolman_camp_get_rates(woolman_camp_next_year());
  $approved = $pending = $waiting = array();
  $form_state['redirect'] = '~camp/register/status';
  $invalid = array('No-show', 'Cancelled', 'Rejected', 'Expired');
  foreach ($vals as $id => $val) {
    if (substr($id, 0, 5) != 'child') {
      continue;
    }
    if (empty($val['questions'])) {
      continue;
    }
    $cid = str_replace('child_', '', $id);
    $child = woolman_contact_get($cid);
    $gender = $child['gender_id'] == 1 ? 'girls' : 'boys';
    $unregistered = array();
    // Registration questions
    foreach ($val['questions'] as $q => &$a) {
      $a = '<h5> ' . $form['child_' . $cid]['questions'][$q]['#title'] . '</h5> ' . nl2br(filter_xss($a));
    }
    $val['questions']['parent_15'] = $parent;
    foreach (woolman_camp_event_types() as $camp => $program) {
      foreach (woolman_aval($val, "register_$camp", array()) as $eid => $checked) {
        if ($checked) {
          $reenroll = FALSE;
          $result = woolman_civicrm_api('participant', 'get', array('contact_id' => $cid, 'event_id' => $eid));
          $params = array(
            'contact_id' => $cid,
            'event_id' => $eid,
            'role_id' => 1,
            'source' => 'Online Registration'
          );
          if ($entity_id = woolman_aval($result, 'id')) {
            if (in_array($result['values'][$entity_id]['participant_status'], $invalid)) {
              $reenroll = TRUE;
              $params['id'] = $entity_id;
            }
          }
          if (!$entity_id || $reenroll) {
            // Register new participant
            if ($sessions[$eid]['full'] || ($rates['gender_balance'] !== '' && $sessions[$eid][$gender] >= ($rates['gender_balance'] / 100 * $sessions[$eid]['max_participants']))) {
              $params['status_id'] = 7;
            }
            elseif (!empty($val['prior_camp'])) {
              $params['status_id'] = 1;
              $approved[$cid] = $child['nick_name'];
            }
            else {
              $params['status_id'] = 8;
              $waiting[$cid] = $child['nick_name'];
            }
            $result = woolman_civicrm_api('participant', 'create', $params);
            $entity_id = $result['id'];
          }
          woolman_custom_field_group_fapi_submit($val['questions'], 7, $entity_id);
        }
        // Un-register camper if unchecked
        elseif ($form[$id]["register_$camp"][$eid]['#default_value'] && empty($form[$id]["register_$camp"][$eid]['#disabled'])) {
          $result = woolman_civicrm_api('participant', 'get', array('contact_id' => $cid, 'event_id' => $eid));
          foreach (woolman_aval($result, 'values', array()) as $par) {
            woolman_civicrm_api('participant', 'create', array(
              'status_id' => 4,
              'id' => $par['id'],
            ));
          }
          $unregistered[] = $sessions[$eid]['title'];
        }
      }
    }
    if ($unregistered) {
      drupal_set_message('You have <em>unregistered</em> ' . $child['nick_name'] . ' from ' . implode(' and ', $unregistered) . '.', 'warning');
    }
  }
  if ($waiting) {
    drupal_set_message('Thanks for registering! Our staff are reviewing your application for '
    . implode(' and ', $waiting) . '. We will contact you very soon (usually within 24 hours) to confirm your registration.');
    // Alert staff
    $message = array(
      'subject' => 'Camp registration by ' . woolman_name('full') . ' needs review',
      'body' => woolman_name('full') . ' has completed the application for ' . implode(' and ', $waiting)
      . ".\nApproval by staff is required to complete registration.\n\nhttp://woolman.org/staff/camp/registration"
    );
    drupal_mail('woolman_website', 0, 'camp@woolman.org', language_default(), $message, 'info@woolman.org');
  }
  if ($approved) {
    drupal_set_message('We are happy to welcome ' . implode(' and ', $approved) . ' back to camp this summer! Please proceed to update the medical forms and pay your deposit to finalize registration.');
    // Alert staff
    $message = array(
      'subject' => 'Camp registration by ' . woolman_name('full') . ' completed',
      'body' => woolman_name('full') . ' has enrolled ' . implode(' and ', $approved)
      . "\n(no approval is needed for returning campers).\n\nhttp://woolman.org/staff/camp/registration"
    );
    drupal_mail('woolman_website', 0, 'camp@woolman.org', language_default(), $message, 'info@woolman.org');
    // Email confirmation
    global $user;
    $to = '"' . woolman_name('full') . '" <' . $user->mail . '>';
    $message = array(
      'subject' => 'Camp registration confirmation',
      'to_cid' => $parent,
      'body' => "Hi " . woolman_name('nick') . ",\n\nWe're so happy to welcome "
      . implode(' and ', $approved) . " back to camp this summer.\n\nThis email is to confirm your registration for the summer of "
      . woolman_camp_next_year() . ". However, please note that your space at camp is not reserved until we receive your deposit. You'll find instructions for making this payment on the same page you registered:\n http:// Camp.woolman.org/register/status\n\nWe're looking forward to seeing "
      . implode(' and ', $approved) . " again!\n\n");
    if ($waiting) {
      $message['body'] .= "We're also excited that you've registered " . implode(' and ', $waiting) . " for the first time! Please give us a few moments to review your application and we'll get back to you with the next steps.\n\n";
    }
    $message['body'] .= "Sincerely,\nYour Friendly Camp Woolman Staff\n530-273-3183x3\nhttp:// Camp.woolman.org";
    drupal_mail('woolman_website', 'create_activity', $to, language_default(), $message, '"Camp Woolman" <camp@woolman.org> ');
  }
  woolman_camp_update_reg_act(2);
}

/**
 * Medical and Release Form Validate
 */
function woolman_camp_med_form_validate($form, &$form_state) {
  $vals = $form_state['values'];
  if (!$vals['permission_for_medical_treatment_35']) {
    form_set_error('permission_for_medical_treatment_35', 'Our safety protocol requires you give permission for medical treatment.');
  }
  if (!$vals['terms_of_agreement_36']) {
    form_set_error('terms_of_agreement_36', 'You must agree to these terms to submit this form.');
  }
}

/**
 * Medical and Release Form Submit
 */
function woolman_camp_med_form_submit($form, &$form_state) {
  $child = $form_state['storage']['child'];
  unset($form_state['storage']);
  $vals =& $form_state['values'];
  $vals['form_submitted_by_37'] = woolman_user_cid();
  $vals['date_form_submitted_38'] = date('Y-m-d H:i:s');
  $vals['ip_address_39'] = ip_address();
  module_load_include('inc', 'woolman_website', 'woolman_website_utils');
  woolman_custom_field_group_fapi_submit($vals, 9, $child['contact_id']);
  drupal_set_message('Saved Medical Info for ' . $child['display_name']);
}

/**
 * Shuttle Form Submit
 */
function woolman_camp_shuttle_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  foreach ($values as $cid => $vals) {
    if (is_numeric($cid) && is_array($vals)) {
      foreach ($vals as $eid => $val) {
        if ($val && empty($form[$cid][$eid]['#default_value'])) {
          woolman_civicrm_api('participant', 'create', array(
            'contact_id' => $cid,
            'event_id' => $eid,
            'role_id' => 1,
            'source' => 'Online Registration'
          ));
        }
        elseif (!$val && !empty($form[$cid][$eid]['#default_value'])) {
          woolman_civicrm_api('participant', 'delete', array('id' => $form[$cid][$eid]['#return_value']));
        }
      }
    }
  }
}
