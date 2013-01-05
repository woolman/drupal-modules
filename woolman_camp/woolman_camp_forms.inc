<?php
/**
 * Forms and form validation functions
 * Part of the Woolman Camp Reg module
 */

module_load_include('inc', 'woolman_camp', 'woolman_camp_form_submit');

/**
 * First Time Form
 */
function woolman_camp_first_time_form(&$form_state) {
  $form = array();
  $form['#prefix'] = '<h3>Welcome to Camp Woolman Registration ' . woolman_camp_next_year()
  . '</h3><p>Hello ' . woolman_name('first') . '</p>';
  $form['message'] = array(
    '#value' => '<p>Thanks for joining us this summer! Please take a moment to let us know how you found out about us, and then it\'s on to our easy registration page.</p>
    <p><em><strong>Important:</strong> If you have registered for camp in the past, you should not be seeing this question. Please ensure you are logged in with the same account you used last year. Feel free to contact us for assistance.</em></p>'
  );
  $form['self_medium'] = array(
    '#type' => 'select',
    '#title' => 'How did you hear about Woolman?',
    '#description' => 'Choose an option',
    '#required' => 1,
    '#options' => woolman_get_civi_options(82),
  );
  $form['self_details'] = array(
    '#type' => 'textfield',
    '#title' => 'Details',
    '#description' => 'Please describe how you heard about us',
    '#required' => TRUE,
  );
  $form['continue'] = array(
    '#type' => 'submit',
    '#value' => 'Continue'
  );
  $form['#redirect'] = '~camp/register/form';
  return $form;
}


/**
 * Navigation Form
 */
function woolman_camp_nav_form(&$form_state, $step = 0) {
  $text = array(
    1 => 'Family Info',
    2 => 'Session Choices',
    3 => 'Confirmation',
  );
  $form = array('#redirect' => '~camp/register/form');

  foreach ($text as $i => $t) {
    if ($i == $step) {
      $status = 'current-step';
    }
    elseif ($i > $step) {
      $status = 'future-step';
    }
    else {
      $status = 'past-step';
    }
    $form['button_' . $i] = array(
      '#type' => 'submit',
      '#value' => $i . ': ' . $t,
      '#disabled' => ($i >= $step),
      '#attributes' => array('class' => $status, 'title' => ($i < $step ? 'Go back to ' . $t.' form' : 'Please complete the form below first'))
    );
    if ($status == 'past-step') {
      $form['button_' . $i]['#attributes']['onclick'] = 'return confirm("Go back and make changes to your ' . strtolower($t) . '?")';
    }
  }
  return $form;
}


/**
 * Family Info Form
 */
function woolman_camp_family_form(&$form_state) {
  drupal_add_js(drupal_get_path('module', 'woolman_camp') . '/js/family_form.js', 'module', 'header', TRUE, FALSE);

  // Get info about entire family
  $self = woolman_camp_contact_get();
  list($children, $spouse, $e_contact, $prior_reg) = woolman_camp_fetch_relations();

  $countries = woolman_get_civi_options('country');
  $phones = array('home' => 'Home', 'cell' => 'Cell', 'work' => 'Work');
  $parent_roles = woolman_get_civi_options(83);

  // Build form
  $form = array('#prefix' => '<h3>Welcome to Camp Woolman Registration ' . woolman_camp_next_year() . '</h3>');
  if ($prior_reg)
    $form['#prefix'] .= '<p>Hi ' . $self['nick_name'] . ', welcome back to another great year at camp! Since you\'ve registered for camp before, we\'ve filled out most of this form for you (pretty cool, huh?).</p><p><strong>But wait!</strong> Don\'t just scroll to the bottom and press Continue. It\'s important that you read through all this very carefully so that we have accurate emergency contact info, and we don\'t make embarrassing mistakes like misspelling your last name!</p><p>So please <em>check everything</em>, update anything that\'s changed, and fill in any missing info.</p>';
  else
    $form['#prefix'] .= '<p>Hi ' . $self['nick_name'] . ', we\'re so glad your family is joining us this year. ' . woolman_camp_next_year() . ' is shaping up to be a fantastic summer, and with your child at camp, it will be even better!</p><p>To get started, please fill out this form completely. The good news? Next year when you come back to register, this will all be filled out for you!</p>';

  $form['self_wrapper'] = array(
    '#prefix' => '<div id="self-wrapper" class="form-wrapper" style="background-color:#C5FAFF;">',
    '#value' => '<h3>Parent/Guardian</h3>',
    '#suffix' => '</div>'
  );
  $form['self_wrapper']['self'] = array(
    '#type' => 'fieldset',
    '#description' => '<p>Please complete the following information about yourself (the parent/guardian).</p>'
  );
  $form['spouse_wrapper'] = array(
    '#prefix' => '<div id="spouse-wrapper" class="form-wrapper" style="background-color:#9BF7B8;">',
    '#value' => '<h3>Your Spouse or Partner</h3>',
    '#suffix' => '</div>'
  );
  if ($spouse) {
    $form['spouse_wrapper']['not_spouse'] = array(
      '#prefix' => '<div class="change-contact">',
      '#suffix' => '</div>',
      '#value' => '<p>Our records show that your partner is <strong id="spouse-name">' . $spouse['display_name'] . '</strong>. If this is not accurate, please click "Change/remove spouse."</p>',
      'not_spouse_checkbox' => array(
        '#type' => 'checkbox',
        '#return_value' => $spouse['contact_id'],
        '#title' => 'Change/remove spouse'
      ),
      'not_spouse_select' => array(
        '#type' => 'select',
        '#title' => '<span>Because ' . $spouse['display_name'] . ' and I</span>',
        '#options' => array('--Please choose an option--','Are now separated or divorced','Are not partners; your records were incorrect'),
      ),
      'spouse_cid' => array('#type' => 'hidden', '#value' => $spouse['contact_id'])
    );
  }
  $form['spouse_wrapper']['spouse'] = array(
      '#type' => 'fieldset',
    );
  $form['spouse_wrapper']['spouse']['spouse_na'] = array(
    '#type' => 'checkbox',
    '#title' => 'Not Applicable',
    '#prefix' => '<div class="na-instructions-wrapper"><p class="na-instructions">' . ($spouse ? 'Please check and update info for <strong>' . $spouse['display_name'] . '</strong></p>':'Please enter your spouse, if applicable.') . '</p>',
    '#suffix' => '</div><div class="clear-block"></div>',
    '#default_value' => !$spouse && ($children || $prior_reg),
  );
  $form['e_contact_wrapper'] = array(
    '#prefix' => '<div id="e-contact-wrapper" class="form-wrapper" style="background-color:#C3E67C;">',
    '#value' => '<h3>Emergency Contact</h3>',
    '#suffix' => '</div>'
  );
  if ($e_contact) {
    $form['e_contact_cid'] = array('#type' => 'hidden', '#value' => $e_contact['contact_id']);
    $form['e_contact_wrapper']['not_e_contact'] = array(
      '#prefix' => '<div class="change-contact">',
      '#suffix' => '</div>',
      '#value' => '<p>Your emergency contact is <strong id="e-contact-name">' . $e_contact['display_name'] . '</strong>. If you wish to have a different emergency contact this year, please click here:</p>',
      'not_e_contact_checkbox' => array(
        '#type' => 'checkbox',
        '#return_value' => $e_contact['contact_id'],
        '#title' => 'Enter different emergency contact'
      ),
    );
  }
  $form['e_contact_wrapper']['e_contact'] = array(
    '#type' => 'fieldset',
  );
  $form['e_contact_wrapper']['e_contact']['e_contact_na'] = array(
    '#type' => 'checkbox',
    '#title' => 'Same as my spouse/partner',
    '#prefix' => '<div class="na-instructions-wrapper"><p class="na-instructions">' . ($e_contact ? 'Please check and update info for <strong>' . $e_contact['display_name'] . '</strong></p>':'Please enter your family\'s emergency contact, if different from your spouse/partner.') . '</p>',
    '#suffix' => '</div><div class="clear-block"></div>',
    '#default_value' => !$e_contact && ($children || $prior_reg),
  );

  foreach (array('self', 'spouse', 'e_contact') as $p) {
    $person = $$p;

    $form[$p . '_wrapper'][$p][$p . '_first'] = array(
      '#prefix' => '<div class="all-fields"><div class="subset">',
      '#type' => 'textfield',
      '#title' => 'First name',
      '#description' => 'Legal first name',
      '#default_value' => woolman_aval($person, 'first_name'),
      '#size' => 15,
      '#req' => 1,
    );
    $form[$p . '_wrapper'][$p][$p . '_nick'] = array(
      '#type' => 'textfield',
      '#title' => 'Nickname',
      '#description' => 'Name ' . ($p=='self'? 'you':'they') . ' go by (if different)',
      '#default_value' => woolman_aval($person, 'nick_name') == woolman_aval($person, 'first_name') ? '' : $person['nick_name'],
      '#size' => 15,
    );
    $form[$p . '_wrapper'][$p][$p . '_middle'] = array(
      '#type' => 'textfield',
      '#title' => 'Middle',
      '#default_value' => woolman_aval($person, 'middle_name'),
      '#size' => 6,
    );
    $form[$p . '_wrapper'][$p][$p . '_last'] = array(
      '#type' => 'textfield',
      '#title' => 'Last name',
      '#default_value' => woolman_aval($person, 'last_name'),
      '#size' => 20,
      '#req' => 1,
    );
    $form[$p . '_wrapper'][$p][$p . '_gender'] = array(
      '#type' => 'radios',
      '#title' => 'Gender',
      '#default_value' => woolman_aval($person, 'gender_id'),
      '#options' => array(1 => 'Female', 2 => 'Male'),
      '#req' => 1,
      '#suffix' => ($p == 'e_contact' ? '</div>' : ''),
    );
    if ($p != 'e_contact') {
      $form[$p . '_wrapper'][$p][$p . '_dob'] = array(
        '#type' => 'date',
        '#title' => 'Birthdate',
        '#default_value' => woolman_aval($person, 'dob', array('year' => 1900,'month' => 1,'day' => 1)),
        '#req' => 1,
        '#suffix' => ($p == 'spouse' ? '' : '</div>'),
      );
    }
    if ($p == 'spouse') {
      $form['spouse_wrapper'][$p]['spouse_relationship'] = array(
        '#type' => 'select',
        '#title' => 'Relationship to You',
        '#options' => woolman_get_civi_options(93),
        '#default_value' => woolman_aval($spouse, 'relationship_custom'),
        '#suffix' => '</div>'
      );
       $form['spouse_wrapper'][$p]['spouse_relationship']['#options']['0'] = 'Divorced';
    }
    if ($p == 'self') {
      $form[$p . '_wrapper'][$p][$p . '_first']['#prefix'] .= '<div class="change-msg">NOTE: You are editing the contact info for yourself here (the parent). Feel free to update/correct your own name, etc. but please <strong>do not</strong> enter anyone other you, the parent, in this space.</div>';
    }
    elseif ($person) {
      $form[$p . '_wrapper'][$p][$p . '_first']['#prefix'] .= '<div class="change-msg">NOTE: You are editing the contact info for ' . $person['display_name'] . ' here. Feel free to update/correct their name, etc. but please don\'t enter a different person without first clicking the "Enter different ' . $p . '" button above.</div>';
    }
    foreach ($phones as $ph=> $phone) {
      $form[$p . '_wrapper'][$p][$p . '_phone_' . $ph] = array(
        '#type' => 'textfield',
        '#prefix' => ($ph=='home'?'<div class="subset">(Please provide at least one phone number)':''),
        '#title' => $phone.' Phone',
        '#size' => 15,
        '#default_value' => woolman_aval($person, "phones:$ph"),
      );
    }
    $form[$p . '_wrapper'][$p][$p . '_phone_primary'] = array(
      '#type' => 'select',
      '#suffix' => '</div>',
      '#title' => 'Primary Phone Number',
      '#options' => $phones,
      '#default_value' => woolman_aval($person, 'phones:primary'),
    );
    $form[$p . '_wrapper'][$p][$p . '_email_home'] = array(
      '#prefix' => '<div class="subset">(Please provide at least one email address)',
      '#type' => 'textfield',
      '#title' => 'Home Email',
      '#default_value' => woolman_aval($person, 'emails:home'),
      '#size' => 25,
    );
    $form[$p . '_wrapper'][$p][$p . '_email_work'] = array(
      '#type' => 'textfield',
      '#title' => 'Work Email',
      '#default_value' => woolman_aval($person, 'emails:work'),
      '#size' => 25,
    );
    $form[$p . '_wrapper'][$p][$p . '_email_primary'] = array(
      '#type' => 'select',
      '#suffix' => '</div>',
      '#title' => 'Primary Email Address',
      '#options' => array('home' => 'Home', 'work' => 'Work'),
      '#default_value' => woolman_aval($person, 'emails:primary'),
    );
    $form[$p . '_wrapper'][$p][$p . '_address'] = array(
      '#type' => 'fieldset',
      '#title' => 'Address',
      '#suffix' => '</div>',
    );
    if ($p != 'self') {
      $form[$p . '_wrapper'][$p][$p . '_address'][$p . '_same'] = array(
        '#type' => 'checkbox',
        '#title' => 'Same as my address',
        '#prefix' => '<div id="sameaddress-' . $p . '" style="float:right;">',
        '#suffix' => '</div>',
        '#default_value' => !empty($self['street_address']) && woolman_aval($person, 'street_address') == $self['street_address'],
        '#attributes' => array('class' => 'same-address'),
      );
    }
    $form[$p . '_wrapper'][$p][$p . '_address'][$p . '_street'] = array(
      '#prefix' => '<div class="subset hide-address" id="' . $p . '-address">',
      '#type' => 'textfield',
      '#title' => 'Street Address',
      '#default_value' => woolman_aval($person, 'street_address'),
      '#size' => 45,
      '#req' => 1,
    );
    $form[$p . '_wrapper'][$p][$p . '_address'][$p . '_city'] = array(
      '#type' => 'textfield',
      '#title' => 'City',
      '#default_value' => woolman_aval($person, 'city'),
      '#size' => 10,
      '#req' => 1,
    );
    $form[$p . '_wrapper'][$p][$p . '_address'][$p . '_state'] = array(
      '#type' => 'textfield',
      '#title' => 'State',
      '#default_value' => woolman_aval($person, 'state_province'),
      '#size' => 3,
      '#maxlength' => 2,
      '#req' => 1,
    );
    $form[$p . '_wrapper'][$p][$p . '_address'][$p . '_zip'] = array(
      '#type' => 'textfield',
      '#title' => 'Zip Code',
      '#default_value' => woolman_aval($person, 'postal_code'),
      '#size' => 8,
      '#req' => 1,
    );
    $form[$p . '_wrapper'][$p][$p . '_address'][$p . '_country'] = array(
      '#type' => 'select',
      '#title' => 'Country',
      '#options' => $countries,
      '#default_value' => woolman_aval($person, 'country_id', USA),
      '#req' => 1,
      '#suffix' => '</div>',
    );
  }
  if (!($c = count($children))) {
    $c = 1;
  }
  for ($i=0; $i<9;) {
    $numbers[$i] = ++$i;
  }
  $form['children_wrapper'] = array(
    '#prefix' => '<div id="children-wrapper" class="form-wrapper" style="background-color:#E1C7C7;">',
    '#value' => '<h3>Children</h3>',
    '#suffix' => '</div>'
  );
  $form['children_wrapper']['number_of_children'] = array(
    '#type' => 'select',
    '#title' => 'How many children do you have?',
    '#options' => $numbers,
    '#default_value' => $c,
    '#suffix' => 'Please complete the name, etc. for each child. You may list them in any order.'
  );
  //Build fieldset for each child
  for ($i = 1; $i<=9; $i++) {
    $form['children_wrapper']['child' . $i] = array(
      '#type' => 'fieldset',
      '#title' => 'Child ' . $i,
    );
    if (!empty($children[$i-1])) {
      $child = $children[$i-1];
      $form['child' . $i . '_cid'] = array('#type' => 'hidden', '#value' => $child['contact_id']);
      if (!empty($child['birth_date'])) {
        $child['dob'] = woolman_date_string_to_array($child['birth_date']);
      }
      $form['children_wrapper']['child' . $i]['#title'] = $child['display_name'];
      $form['children_wrapper']['child' . $i]['#description'] = '<p>Please verify and update info for <strong>' . $child['display_name'] . '</strong>.</p><div class="change-msg">NOTE: You are editing the contact info for ' . $child['display_name'] . ' here. Feel free to update/correct their name, etc. but please <strong>do not</strong> enter a different person in this space. To add more children, increase the number above.</div>';
    }
    else {
      $child = array();
    }
    $form['children_wrapper']['child' . $i]['child' . $i . '_first'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => 'First name',
      '#default_value' => woolman_aval($child, 'first_name'),
      '#size' => 15,
      '#mark_req' => 1,
    );
    $form['children_wrapper']['child' . $i]['child' . $i . '_nick'] = array(
      '#type' => 'textfield',
      '#title' => 'Nickname',
      '#default_value' => woolman_aval($child, 'nick_name') == woolman_aval($child, 'first_name') ? '' : $child['nick_name'],
      '#size' => 12,
    );
    $form['children_wrapper']['child' . $i]['child' . $i . '_middle'] = array(
      '#type' => 'textfield',
      '#title' => 'Middle',
      '#default_value' => woolman_aval($child, 'middle_name'),
      '#size' => 6,
    );
    $form['children_wrapper']['child' . $i]['child' . $i . '_last'] = array(
      '#type' => 'textfield',
      '#title' => 'Last name',
      '#default_value' => woolman_aval($child, 'last_name'),
      '#size' => 20,
      '#mark_req' => 1,
    );
      $form['children_wrapper']['child' . $i]['child' . $i . '_dob'] = array(
      '#type' => 'date',
      '#title' => 'Birthdate',
      '#default_value' => woolman_aval($child, 'dob', array('year' => 1900, 'month' => 1, 'day' => 1)),
      '#suffix' => '</div>',
      '#mark_req' => 1,
    );
    $form['children_wrapper']['child' . $i]['child' . $i . '_email'] = array(
      '#type' => 'textfield',
      '#title' => 'Email (optional)',
      '#default_value' => woolman_aval($child, 'email'),
      '#size' => 20,
      '#prefix' => '<div class="subset">',
    );
    $form['children_wrapper']['child' . $i]['child' . $i . '_gender'] = array(
      '#type' => 'radios',
      '#title' => 'Gender',
      '#default_value' => woolman_aval($child, 'gender_id'),
      '#options' => array(1 => 'Female', 2 => 'Male'),
      '#mark_req' => 1,
    );
    $form['children_wrapper']['child' . $i]['child' . $i . '_relationship'] = array(
      '#type' => 'select',
      '#title' => 'Your Parental Role',
      '#options' => $parent_roles,
      '#default_value' => woolman_aval($child, 'relationship_custom'),
      '#mark_req' => 1,
    );
    $form['children_wrapper']['child' . $i]['child' . $i . '_relationship_spouse'] = array(
      '#type' => 'select',
      '#title' => '<span class="spouse-name-here">Your Partner/Spouse</span>\'s Parental Role',
      '#options' => $parent_roles,
      '#default_value' => woolman_aval($child, 'relationship_to_spouse'),
      '#suffix' => '</div>',
      '#mark_req' => 1,
    );
  }
  $form['#redirect'] = '~camp/register/form';
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save and Continue'
  );
  return $form;
}

/**
 * Form for Choosing Camp Sessions to Attend
 */
function woolman_camp_registration_form(&$form_state) {
  drupal_add_css('sites/all/libraries/jquery.ui/themes/default/ui.all.css', 'module', 'all', FALSE);
  jquery_ui_add(array('ui.tabs'));
  drupal_add_js(drupal_get_path('module', 'woolman_camp') . '/js/reg_form.js', 'module', 'header', TRUE, FALSE);
  module_load_include('inc', 'woolman_website', 'woolman_website_utils');

  // Info about entire family
  list($children, $spouse, $e_contact, $prior_reg) = woolman_camp_fetch_relations();

  $year = woolman_camp_next_year();
  $rates = woolman_camp_get_rates($year);
  $session_data = woolman_camp_sort_sessions(woolman_camp_get_sessions($year));
  $programs = woolman_camp_event_types();
  $age_limit = woolman_camp_age_limits(1);

  $form = array('#prefix' => '<h2>Camper Registration Form</h2><p>Thanks for updating your family info. The next step is to decide what dates your ' . (count($children)==1 ? 'child is' : 'children are') . ' coming for camp. Please use the following guidelines:</p><p><strong>Mini Camp:</strong>. A 2-day experience for 6 to 9 year-olds.</p><p><strong>Camp Woolman:</strong> for children between the ages of 9 and 14. Campers may stay for 1, 2, 3, or 4 weeks by combining multiple sessions. Note: Campers will need to go home in-between sessions that are not part of an A-B combo.</p><p><strong>Woolman Teen Leadership Camp:</strong> for 15 and 16 year-olds. Campers should register for a single 2-week session.</p><p><strong>Age Exceptions:</strong> We can make exceptions for campers who are a little too young or old for a session. If choosing such a session, please indicate why in your application below.</p>');

  $form['tabs'] = array('#value' => '<ul id="tab-nav">');

  $invalid = array('No-show', 'Cancelled', 'Rejected', 'Expired');
  foreach ($children as $child) {
    $id = 'child_' . $child['contact_id'];
    // Camper age, with exception applied
    $age = woolman_camp_age($child['birth_date'], $year, $child['contact_id']);
    $his_her = $child['gender_id'] == 1 ? 'her' : 'his';
    $him_her = $child['gender_id'] == 1 ? 'her' : 'him';
    $he_she = $child['gender_id'] == 1 ? 'she' : 'he';
    $gender = $child['gender_id'] == 1 ? 'girls' : 'boys';

    $form['tabs']['#value'] .= '<li><a href="#child-' . $child['contact_id'] . '"><span class="child-name">' . $child['nick_name'] . ' ' . $child['last_name'] . '</span><span class="registered-for">&nbsp;</span></a></li>';

    $form[$id] = array(
      '#prefix' => '<div id="child-' . $child['contact_id'] . '" class="child-wrapper">',
      '#value' => '<div class="child-name">' . $child['nick_name'] . ' ' . $child['last_name'] . '</div><div class="child-age">Age This July: ' . woolman_camp_age($child['birth_date'], $year) . '</div>',
      '#suffix' => '<div style="clear:both"></div></div>',
      '#tree' => TRUE,
      'prior_camp' => array('#type' => 'hidden', '#value' => 0)
    );
    if ($age > $age_limit['min'] && $age < $age_limit['max']) {
      $last_year = $participant_id = $submission = NULL;
      $current_reg = array();
      if (!empty($child['prior_camp'])) {
        foreach ($child['prior_camp'] as $d => $e) {
          $y = substr($d, 0, 4);
          if ($y == $year && !in_array($e['participant_status'], $invalid)) {
            $current_reg[] = $e['event_id'];
            $participant_id = $e['participant_id'];
          }
          elseif ($y < $year && !in_array($e['participant_status'], $invalid)) {
            $form[$id]['prior_camp']['#value'] = 1;
            if ($y == $year - 1) {
              $last_year = 'last summer';
            }
            else {
              $last_year = "summer $y";
            }
            break;
          }
        }
      }

      foreach ($session_data as $camp => $sessions) {
        if (!woolman_camp_age_range($age, $camp, 1)) {
          continue;
        }
        $form[$id]["register_$camp"] = array(
          '#prefix' => '<div class="register-wrapper">',
          '#suffix' => '<div class="clear-block"></div></div>',
          '#value' => '<h4>' . $programs[$camp]['name'] . ' (ages ' . $programs[$camp]['age_min'] . '-' . $programs[$camp]['age_max'] . ')</h4>',
        );
        foreach ($sessions as $s) {
          $title = trim(str_replace("$year", '', $s['title']));
          if (substr($s['start_date'], 0, 10) < date('Y-m-d')) {
            $action = 'Register';
            $status = 'Past';
            $class = 'ended';
          }
          elseif ($s['full'] || ($rates['gender_balance'] !== '' && $s[$gender] >= ($rates['gender_balance'] / 100 * $s['max_participants']))) {
            $action = 'Join waitlist';
            $status = 'Sorry, full' . ($s['full'] ? '' : ' for ' . $gender);
            $class = 'closed';
          }
          else {
            $action = 'Register';
            $status = 'Space Available';
            $class = 'open';
          }
          $form[$id]["register_$camp"][$s['id']] = array(
            '#type' => 'checkbox',
            '#title' => $action,
            '#disabled' => $class == 'ended',
            '#default_value' => in_array($s['id'], $current_reg),
            '#prefix' => '<div class="register-button ' . $class . '">
                            <h5 class="session-title">' . $title . '</h5>
                            <div class="session-dates">' .
                              woolman_camp_date_format($s['start_date'], $s['end_date']) . '
                            </div>',
            '#suffix' => '  <div class="session-status">
                              ' . $status.'
                            </div>
                          </div>',
          );
        }
      }
      $form[$id]['questions'] = woolman_custom_field_group_fapi(7, $participant_id);
      $form[$id]['questions']['#prefix'] = '<div class="questions-wrapper">';
      $form[$id]['questions']['#suffix'] = '</div>';
      $form[$id]['questions']['#value'] = '<h2>Registration Questions</h2>';
      $form[$id]['questions']['camper_question_1_10']['#prefix'] = '<h4 style="margin-bottom:0.2em;">For the Camper</h4><em>Please ask ' . $child['nick_name'] . ' to answer these questions (you may skip these and return later if needed):</em>';
      $form[$id]['questions']['camper_question_1_10']['#title'] = 'What are you looking forward to about camp?';
      $form[$id]['questions']['camper_question_2_12']['#title'] = 'Is there anything you are nervous about?';
      $form[$id]['questions']['parent_question_1_13']['#prefix'] = '<h4>Questions for Parents</h4>';
      if ($last_year) {
        $question = "Has anything changed in {$child['nick_name']}'s life since $last_year that we should know about? (i.e. in $his_her health, family life, friends, school, diet, etc.)";
      }
      else {
        $question = "What experiences has {$child['nick_name']} had of being away from home or going camping?<br />How did $he_she do in those situations?<br />How do you think $he_she will do at camp this summer and what issues do you think might come up?";
      }
      $form[$id]['questions']['parent_question_1_13']['#title'] = $question;
      $form[$id]['questions']['parent_question_2_14']['#title'] = "What physical, emotional, social, or dietary needs does {$child['nick_name']} have? Please include any allergies, food restrictions, medications, behavioral issues, physical limitations, or anything else we should know to make sure $he_she gets $his_her needs met at camp.";
      unset($form[$id]['questions']['parent_15'], $form[$id]['questions']['camper_question_1_10']['#description'], $form[$id]['questions']['parent_question_1_13']['#description']);
      foreach ($form[$id]['questions'] as &$q) {
        if (!empty($q['#default_value'])) {
          $q['#default_value'] = str_replace(array('<br />', '<p>', '</p>'), '', $q['#default_value']);
          $q['#default_value'] = str_replace('  ', ' ', $q['#default_value']);
          if ($p = strrpos($q['#default_value'], '</h5>')) {
            $q['#default_value'] = trim(substr($q['#default_value'], $p+5));
          }
          elseif ($p = strrpos($q['#default_value'], '&lt;/h5&gt;')) {
            $q['#default_value'] = trim(substr($q['#default_value'], $p+11));
          }
        }
      }
    }
    else {
      if ($age > $age_limit['max'] && $age < 19) {
        $message = 'Sadly, ' . $child['nick_name'] . ' is too old to be a camper, but did you know about our Counselor in Training program?';
      }
      elseif ($age == $age_limit['min']) {
        $message = 'Almost there! ' . $child['nick_name'] . ' will be old enough for mini camp next year.';
      }
      elseif ($age < $age_limit['min']) {
        $message = $child['nick_name'] . ' will be old enough for mini camp in another ' . ($age_limit['min'] - $age) . ' years!';
      }
      else {
        $message = 'Sadly, ' . $child['nick_name'] . ' is too old for camp.';
      }
      $form[$id]['register'] = array(
        '#prefix' => '<div class="no-register-wrapper">',
        '#value' => $message,
        '#suffix' => '</div>'
      );
    }
  }
  $form['tabs']['#value'] .= '</ul>';
  $form['#redirect'] = '~camp/register/form';
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save and Continue',
    '#prefix' => '<div id="submit-reg-form">' .
    (count($children) > 1 ? '<p id="camper-tab-tip">Click Each Child To Register</p>' : '') .
    '<p id="total"></p><p>All done? Click here to go to the final step.</p>',
    '#suffix' => '</div>',
  );
  return $form;
}

/**
 * Medical and Release Form
 */
function woolman_camp_med_form(&$form_state, $cid) {
  $self = woolman_user_cid();
  // Verify relationship with child
  if (!db_result(db_query('SELECT id FROM {civicrm_relationship} WHERE contact_id_a = %d AND contact_id_b = %d AND relationship_type_id = 1', $cid, $self))) {
    return array();
  }
  $form_state['storage']['child'] = $child = woolman_contact_get($cid);
  drupal_set_title('Emergency Medical Information for ' . $child['display_name']);
  module_load_include('inc', 'woolman_website', 'woolman_website_utils');
  $form = woolman_custom_field_group_fapi(9, $cid);
  $form['#prefix'] = '<p>Please fill out the following information as completely as possible.</p>';
  $form['#redirect'] = '~camp/register/status';
  $form['terms_of_agreement_36']['#title'] = 'I agree to the following terms';
  $form['height_24']['#prefix'] = '<div class="subset">';
  $form['weight_25']['#suffix'] = '</div>';
  $form['medical_conditions_symptoms_26']['#theme'] = $form['over_the_counter_medications_48']['#theme'] = array('multicolumn_options');
  $form['medical_conditions_symptoms_26']['#columns'] = $form['over_the_counter_medications_48']['#columns'] = 2;
  $form['photo_release_34']['#description'] .= '<br /><strong>Note:</strong> <em>Censoring photos is hard on our staff, and not allowing people to be in group photos is hard on the campers. We always display pictures of children in tasteful, positive ways, and we ask that you please give us permission to include your child unless you have important reasons for not doing so.</em>';
  unset($form['form_submitted_by_37'], $form['date_form_submitted_38'], $form['ip_address_39']);
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit Form'
  );
  foreach (array('height_24', 'weight_25', 'swimming_ability_33', 'photo_release_34', 'permission_for_medical_treatment_35', 'terms_of_agreement_36', 'physical_ability_level_47') as $id) {
    $form[$id]['#required'] = TRUE;
  }
  return $form;
}

/**
 * Shuttle Form
 */
function woolman_camp_shuttle_form(&$form_state) {
  $year = woolman_camp_next_year();
  $rates = woolman_camp_get_rates($year);
  $shuttle = woolman_camp_get_shuttle($year, TRUE);
  foreach (array('coming', 'going') as $dir) {
    $$dir = array();
    foreach ($shuttle[$dir] as $trip) {
      ${$dir}[substr($trip['start_date'], 0, 10)] = $trip;
    }
  }
  list($kids, $spouse, $e_contact, $past_reg) = woolman_camp_fetch_relations();
  $sessions = woolman_camp_get_sessions($year);

  $form = array('#prefix' => 'Shuttle service is between the Bay Area and camp. The cost is $' . $rates['shuttle_price'] . ' per ride (one-way) . ');
  foreach ($kids as $kid) {
    if (!isset($kid['prior_camp'])) {
      continue;
    }
    $cid = $kid['contact_id'];
    $shuttles = array();
    $skip = TRUE;
    foreach ($kid['prior_camp'] as $date => $sess) {
      if(substr($date, 0, 4) == $year) {
        $skip = FALSE;
        if (array_key_exists(substr($date, 0, 10), $coming)) {
          $shuttles[] = $coming[substr($date, 0, 10)];
        }
        if (array_key_exists(substr($sess['event_end_date'], 0, 10), $going)) {
          $shuttles[] = $going[substr($sess['event_end_date'], 0, 10)];
        }
      }
    }
    if (!$skip) {
      $form[$cid] = array(
        '#type' => 'fieldset',
        '#tree' => TRUE,
        '#title' => $kid['display_name'],
      );
      if ($shuttles) {
        foreach ($shuttles as $shuttle) {
          $form[$cid][$shuttle['event_id']] = array(
            '#type' => 'checkbox',
            '#title' => $shuttle['title'],
            '#description' => '<strong>' . date('M jS', strtotime($shuttle['start_date'])) . ':</strong> ' . nl2br($shuttle['description']),
            '#default_value' => isset($shuttle['campers'][$cid]),
            '#return_value' => woolman_aval($shuttle, "campers:$cid:participant_id", 1),
          );
          if(count($shuttle['campers']) >= $shuttle['max_participants']) {
            $form[$cid][$shuttle['event_id']]['#disabled'] = TRUE;
            $form[$cid][$shuttle['event_id']]['#description'] = 'Sorry, there is no more room on this shuttle';
          }
        }
      }
      else {
        $form[$kid['contact_id']]['#description'] = 'Sorry, no shuttles are running for those camp sessions.';
      }
    }
  }
  $form['#redirect'] = '~camp/register/status';
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save'
  );
  return $form;
}

/**
 * Add camp info to CiviCRM contribution form
 */
function woolman_camp_contribution_form($formName, &$form) {
  drupal_add_css(drupal_get_path('module', 'woolman_camp') . '/css/camp_forms.css');
  $year = woolman_camp_next_year();
  $intro = '<div id="woolman-camp-contribution">';
  if ($cid = woolman_user_cid()) {
    if ($reg = woolman_camp_calc_fees($cid, $year)) {
      $balance = $reg['total_fees'] - $reg['total_discount'] - $reg['total_paid'];
      $intro .= '<table><caption><strong>Your Camp Fees</strong></caption><tbody>
      <tr><td>Camp Tuition and Fees</td><td>$' . $reg['total_fees'] . '</td></tr>
      <tr style="font-style:italic;"><td>Discounts:</td><td class="p">-$' . $reg['total_discount'] . '</td></tr>
      <tr style="font-weight:bold;"><td>Total Fees</td><td>$' . ($reg['total_fees'] - $reg['total_discount']) . '</td></tr>';
      $intro .= '<tr><td>Paid to-date</td><td>$' . $reg['total_paid'] . '</td></tr>';
      $intro .= '<tr style="font-weight:bold;"><td>Balance Due</td><td>$' . $balance . '</td></tr>';
      $intro .= '</tbody></table><p>';
      $defaults = array('amount_other' => $balance);
      $form->setDefaults($defaults);
      if ($balance <= 0) {
        $intro .= 'Thank you, your tuition and fees are already paid in full. ' . l('Click here to return to your camp registration page.', '~camp/register/status', array('alias' => TRUE));
      }
      elseif ($reg['total_paid'] >= $reg['deposit']) {
        $intro .= 'You may pay all or part of your remaining balance of $' . $balance . '. The full amount is due on the first day of camp.';
      }
      else {
        $intro .= 'You may pay all fees now, or just the $' . $reg['deposit'] . ' nonrefundable deposit required to reserve your child\'s place at camp. The full $' . $balance . ' is due on the first day of camp.';
      }
      $intro .= '</p>';
    }
  }
  else {
    $intro .= '<p>Please ' . l('Log In', 'user', array('query' => drupal_get_destination(), 'alias' => TRUE)) . ' to see your camp balance.</p>';
  }
  $intro .= '</div>';
  $form->assign('intro_text', $intro);
}
