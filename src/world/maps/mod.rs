use crate::assets::Assets;
use crate::engine::state::GameState;

pub mod city;
pub mod iwakura;
pub mod neighborhood;
pub mod school;
pub mod yamanote;

pub fn init_map(assets: &mut Assets, _state: &GameState) {
    let s = |id: &str, _fallback: &str| -> String {
        assets.get_string(id).to_string()
    };

    let mut all = Vec::new();
    all.extend(yamanote::build_yamanote());
    all.extend(iwakura::build_iwakura(&s));
    all.extend(neighborhood::build_neighborhood(&s));
    all.extend(school::build_school(&s));
    all.extend(city::build_city(&s));

    for loc in all {
        assets.locations.insert(loc.id.clone(), loc);
    }
}
