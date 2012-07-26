<?php
/**
 * Displays a page about High Schools our students have come from
 * Part of the Custom Woolman module
 */

/**
 * Retrieve Sending Schools
 */
function woolman_sending_schools() {

  if ($output = cache_get('woolman_sending_schools')) {
    return views_embed_view('embed_image', 'attachment_1', 111, '3+4+5') . $output->data;
  }

  $resource = db_query(
   "SELECT con.id as cid, con.display_name, st.name as state, st.country_id as st_co_id, cnt.name as country
    FROM {civicrm_contact} con
    JOIN {civicrm_address} addr ON addr.contact_id = con.id
    LEFT JOIN {civicrm_country} cnt ON cnt.id = addr.country_id
    LEFT JOIN {civicrm_state_province} st ON st.id = addr.state_province_id
    WHERE addr.is_primary = 1
    AND con.contact_sub_type = 'High_School'
    AND con.id IN (
      SELECT contact_id_b FROM {civicrm_relationship}
      WHERE relationship_type_id = 10
      AND contact_id_a IN (
        SELECT contact_id_a FROM {civicrm_relationship}
        WHERE relationship_type_id = 10
        AND contact_id_b = " . WOOLMAN ."
        )
      )
    ORDER BY display_name"
  );
  $schools = array();
  while ($school = db_fetch_array($resource)) {

    if (!$school['country'] && $school['st_co_id'] == 1228)
      $school['country'] = 'United States';

    if ($school['country'] == 'United States')
      $schools[$school['state']][$school['cid']] = $school['display_name'];
    elseif ($school['country'])
      $schools[$school['country']][$school['cid']] = $school['display_name'];
  }
  if (!$schools) return 'Error';

  ksort($schools);

  $output = theme_woolman_sending_schools($schools);
  //cache this output for 20 days
  cache_set('woolman_sending_schools', $output, 'cache', time() + (20 * 24 * 60 * 60));
  // Add a page image
  return views_embed_view('embed_image', 'attachment_1', 111, '3+4+5') . $output;
}

/**
 * Theme Sending Schools
 */
function theme_woolman_sending_schools($schools) {
  //estimate size of columns - a heading is about 3x bigger than a listing
  $total = 0;
  foreach ($schools as $sch) {
    $total += count($sch)+3;
  }

  $div = 1;
  $count = 0;
  $output = '<h5>These High Schools have all sent students to the Woolman Semester:</h5><div id="sending-schools-1" style="float:left; width:40%;">';
  foreach ($schools as $area=> $sch) {
    if (($count+=3) > ($total/2) && $div == 1) {
      $div = 2;
      $output .= '</div><div id="sending-schools-2" style="float:left;">';
    }
    $output .= '<h4>' . $area.'</h4><ul>';
    foreach ($sch as $s) {
      ++$count;
      $output .= '<li>' . $s.'</li>';
    }
    $output .= '</ul>';
  }
  $output .= '</div>';
  return $output;
}
