use LWP;

my $ua = LWP::UserAgent->new;
$ua->agent("TestApp/0.1 ");
$ua->env_proxy();

my $req = HTTP::Request->new(POST => 'https://www.google.com/loc/json');

$req->content_type('application/jsonrequest');
$req->content('{"cell_towers": [{"location_area_code": "22295", "mobile_network_code": "0", "cell_id": "16126", "mobile_country_code": "460"},{"location_area_code": "22295", "mobile_network_code": "0", "cell_id": "30413", "mobile_country_code": "460"}], "version": "1.1.0", "request_address": "true"}');

my $res = $ua->request($req);

if ($res->is_success) {
    print $res->content;
} else {
    print $res->status_line, "\n";
    return undef;
}

