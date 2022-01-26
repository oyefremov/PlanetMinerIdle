// MinerEstimator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <array>
#include <chrono>
#include <cassert>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <format>

using namespace std::chrono_literals;
using namespace std::chrono;
using namespace std;

#define ADD_COMMA(X) X,
#define STR(X) #X,

#define LIST_NONE(ITEM) \
    ITEM(NullResource)


// List ore names
#define LIST_ORE(ITEM) \
    ITEM(Copper     )\
    ITEM(Iron       )\
    ITEM(Lead       )\
    ITEM(Silica     )\
    ITEM(Aluminium  )\
    ITEM(Silver     )\
    ITEM(Gold       )\
    ITEM(Diamond    )\
    ITEM(Platinum   )\
    ITEM(Titanium   )\
    ITEM(Iridium    )\
    ITEM(Paladium   )\
    ITEM(Osmium     )\
    ITEM(Rhodium    )\
    ITEM(Inerton    )\
    ITEM(Quadium    )


#define LIST_ALLOYS(ITEM) \
    ITEM(CopperBar)\
    ITEM(IronBar)\
    ITEM(LeadBar)\
    ITEM(SilicaBar)\
    ITEM(AluminiumBar)\
    ITEM(SilverBar)\
    ITEM(GoldBar)\
    ITEM(BronzeBar)\
    ITEM(SteelBar)\
    ITEM(PlatinumBar)\
    ITEM(TitaniumBar)\
    ITEM(IridiumBar)\
    ITEM(PaladiumBar)\
    ITEM(OsmiumBar)\
    ITEM(RhodiumBar)\
    ITEM(InertonBar)\
    ITEM(QuadiumBar)

// List items
#define LIST_ITEMS(ITEM) \
    ITEM(CopperWire      )\
    ITEM(IronNails       )\
    ITEM(Battery         )\
    ITEM(Hammer          )\
    ITEM(Glass           )\
    ITEM(Circuit         )\
    ITEM(Lense           )\
    ITEM(Laser           )\
    ITEM(BasicComputer   )\
    ITEM(SolarPanel      )\
    ITEM(LaserTorch      )\
    ITEM(AdvancedBattery )\
    ITEM(ThermalScanner  )\
    ITEM(AdvancedComputer)\
    ITEM(NavigationModule)\
    ITEM(PlasmaTorch     )\
    ITEM(RadioTower      )\
    ITEM(Telescope       )\
    ITEM(SatelliteDish   )\
    ITEM(Motor           )\
    ITEM(Accumulator     )\
    ITEM(NuclearCapsule  )\
    ITEM(WindTurbine     )\
    ITEM(SpaceProbe	     )\
    ITEM(NuclearReactor  )\
    ITEM(Collider        )\
    ITEM(GravityChamber  )


enum ResourceId{
    LIST_NONE(ADD_COMMA)
    LIST_ORE(ADD_COMMA)
    LIST_ALLOYS(ADD_COMMA)
    LIST_ITEMS(ADD_COMMA)
    ResourceSize,
    MaxResource = ResourceSize - 1
};

std::array<const char*, ResourceSize> names{
    LIST_NONE(STR)
    LIST_ORE(STR)
    LIST_ALLOYS(STR)
    LIST_ITEMS(STR)
};

// config:

auto best_ore = Rhodium;
auto best_alloy = TitaniumBar;
auto best_item = NavigationModule;

double alloy_ingredients_multiplier = (1.0 - 0.38); // *0.8;
double item_ingredients_multiplier = 0.5; // *0.8;

double alloy_value_multiplier = 1.0;
double item_value_multiplier = 1.0;

vector<pair<double, ResourceId>> sorted_alloys;


struct ResCount {
    ResourceId id;
    uint32_t count;

    friend bool operator<(const ResCount& a, const ResCount& b) {
        return a.id < b.id || a.id == b.id && a.count < b.count;
    }
};

ResCount count_ingredients(ResCount rc, double k) {
    // 0.5 must be round down so that round(5 * 0.5) -> 2
    rc.count = static_cast<uint32_t>(ceil(rc.count * k - 0.5));
    if (rc.count == 0) rc.count = 1;
    return rc;
}

struct Resource {
    ResourceId id = NullResource;
    bool is_ore = false;
    bool is_alloy = false;
    bool is_item = false;
    seconds smelt_time = 0s;
    seconds craft_time = 0s;
    double sell_price = 0;
    double price_multiplier = 1.0;
    int stars = 0;
    std::vector<ResCount> required_resources;

    const char* name() const { return names[(int)id]; }
    double required_resources_multiplier() const { return is_item ? item_ingredients_multiplier : is_alloy ? alloy_ingredients_multiplier : 1.0; }
};

Resource make_ore(ResourceId id, int price) {
    Resource ore;
    ore.id = id;
    ore.is_ore = true;
    ore.sell_price = price;
    return ore;
}


Resource make_alloy(ResourceId id, int price, seconds smelt_time, std::vector<ResCount> required_resources) {
    Resource alloy;
    alloy.id = id;
    alloy.is_alloy = true;
    alloy.smelt_time = smelt_time;
    alloy.sell_price = price;
    alloy.required_resources = std::move(required_resources);
    return alloy;
}

Resource make_bar(ResourceId id, int price, seconds smelt_time, ResourceId base_ore) {
    return make_alloy(id, price, smelt_time, { ResCount{base_ore, 1000} });
}

Resource make_item(ResourceId id, uint64_t price, seconds craft_time, std::vector<ResCount> required_resources) {
    Resource item;
    item.id = id;
    item.is_item = true;
    item.craft_time = craft_time;
    item.sell_price = (double)price;
    item.required_resources = std::move(required_resources);
    return item;
}

Resource make_item(ResourceId id, uint64_t price, seconds craft_time, ResourceId res, uint32_t count) {
    return make_item(id, price, craft_time, { {res, count } });
}

Resource make_item(ResourceId id, uint64_t price, seconds craft_time, ResourceId res1, uint32_t count1, ResourceId res2, uint32_t count2) {
    return make_item(id, price, craft_time, { {res1, count1 }, {res2, count2} });
}

Resource make_item(ResourceId id, uint64_t price, seconds craft_time, ResourceId res1, uint32_t count1, ResourceId res2, uint32_t count2, ResourceId res3, uint32_t count3) {
    return make_item(id, price, craft_time, { {res1, count1 }, {res2, count2} , {res3, count3} });
}

std::vector<Resource> all_ore() {
    return {
        make_ore(Copper, 1),
        make_ore(Iron, 2),
        make_ore(Lead, 4),
        make_ore(Silica, 8),
        make_ore(Aluminium, 17),
        make_ore(Silver, 36),
        make_ore(Gold, 75),
        make_ore(Diamond, 160),
        make_ore(Platinum, 340),
        make_ore(Titanium, 730),
        make_ore(Iridium, 1'600),
        make_ore(Paladium, 3'500),
        make_ore(Osmium, 7'800),
        make_ore(Rhodium, 17'500),
        make_ore(Inerton, 40'000),
        make_ore(Quadium, 92'000),
    };
}

std::vector<Resource> all_alloy() {
    return {
        make_bar(CopperBar, 1450, 20s, Copper),
        make_bar(IronBar, 3000, 30s,       Iron),
        make_bar(LeadBar, 6100, 40s,       Lead),
        make_bar(SilicaBar, 12'500, 1min,    Silica),
        make_bar(AluminiumBar, 27'600, 80s,Aluminium),
        make_bar(SilverBar, 60'000, 2min,    Silver),
        make_bar(GoldBar, 120'000, 3min,      Gold),
        make_alloy(BronzeBar, 234'000, 4min, {{SilverBar, 2}, {CopperBar, 10}}),
        make_alloy(SteelBar, 340'000, 8min,  {{LeadBar, 15}, {IronBar, 30}}),
        make_alloy(PlatinumBar, 780'000, 10min, {{GoldBar, 2}, {Platinum, 1000}}),
        make_alloy(TitaniumBar, 1'630'000, 12min, {{BronzeBar, 2}, {Titanium, 1000}}),
        make_alloy(IridiumBar, 3'110'000, 14min, {{SteelBar, 2}, {Iridium, 1000}}),
        make_alloy(PaladiumBar, 7'000'000, 16min, {{PlatinumBar, 2}, {Paladium, 1000}}),
        make_alloy(OsmiumBar, 14'500'000, 18min, {{TitaniumBar, 2}, {Osmium, 1000}}),
        make_alloy(RhodiumBar, 31'000'000, 20min, {{IridiumBar, 2}, {Rhodium, 1000}}),
        make_alloy(InertonBar, 250'000'000, 24min, {{PaladiumBar, 2}, {Inerton, 1000}}),
        make_alloy(QuadiumBar, 500'000'000, 28min, {{OsmiumBar, 2}, {Quadium, 1000}}),
    };
}

std::vector<Resource> all_items() {
    return {        
        make_item(CopperWire, 10'000, 1min, CopperBar, 5),
        make_item(IronNails, 20'000, 2min, IronBar, 5),
        make_item(Battery, 70'000, 4min, CopperWire, 2, CopperBar, 10),
        make_item(Hammer, 135'000, 6min, IronNails, 2, LeadBar, 5),
        make_item(Glass, 220'000, 12min, SilicaBar, 10),
        make_item(Circuit, 620'000, 20min, CopperWire, 10, AluminiumBar, 5, SilicaBar, 5),
        make_item(Lense, 1'100'000, 40min, Glass, 1, SilverBar, 5),
        make_item(Laser, 3'200'000, 60min, GoldBar, 5, Lense, 1, IronBar, 10),
        make_item(BasicComputer, 7'600'000, 80min, Circuit, 5, SilverBar, 5),
        make_item(SolarPanel, 12'500'000, 100min, Circuit, 5, Glass, 10),
        make_item(LaserTorch, 31'000'000, 120min, BronzeBar, 5, Laser, 2, Lense, 5),
        make_item(AdvancedBattery, 35'000'000, 150min, SteelBar, 20, Battery, 30),
        make_item(ThermalScanner, 71'500'000, 10800s, PlatinumBar, 5, Laser, 2, Glass, 5),
        make_item(AdvancedComputer, 180'000'000, 12600s, BasicComputer, 5, TitaniumBar, 5),
        make_item(NavigationModule, 1'000'000'000, 13500s, LaserTorch, 2, ThermalScanner, 1),
        make_item(PlasmaTorch, 1'150'000'000, 15000s, IridiumBar, 15, LaserTorch, 5),
        make_item(RadioTower, 1'450'000'000, 15600s, PlatinumBar, 75, AluminiumBar, 150, TitaniumBar, 50),
        make_item(Telescope, 2'700'000'000, 16800s, Lense, 20, AdvancedComputer, 1),
        make_item(SatelliteDish, 3'400'000'000, 18000s, SteelBar, 150, PaladiumBar, 20),
        make_item(Motor, 7'000'000'000, 19200s, BronzeBar, 500, Hammer, 200),
        make_item(Accumulator, 12'000'000'000, 20'400s, OsmiumBar, 20, AdvancedBattery, 2),
        make_item(NuclearCapsule, 26'000'000'000, 21000s, RhodiumBar, 5, PlasmaTorch, 1),
        make_item(WindTurbine, 140'000'000'000, 21600s, AluminiumBar, 150, Motor, 1),
        make_item(SpaceProbe,        180'000'000'000, 22200s, SatelliteDish, 1, Telescope, 1, SolarPanel, 25),
        make_item(NuclearReactor,  1'000'000'000'000, 22800s, NuclearCapsule, 1, IridiumBar, 300),
        make_item(Collider,        2'000'000'000'000, 23100s, Inerton, 500, Quadium, 100),
        make_item(GravityChamber, 15'000'000'000'000, 24300s, AdvancedComputer, 60, NuclearReactor, 1),

    };
}

std::array<Resource, ResourceSize> init_resources(){
    std::array<Resource, ResourceSize> res;
    for (auto& group : {all_ore(), all_alloy(), all_items()})
        for (auto& r : group) {
            assert(r.id != NullResource);
            assert(res[r.id].id == NullResource);
            res[r.id] = r;
        }
    for (auto& r : res) {
        if (&r == &res[0] || r.id != NullResource) continue;
        cout << names[&r - &res[0]] << " not defined\n";
        assert(&r == &res[0] || r.id != NullResource);
    }   

    // stars
    res[Copper].stars = 5;
    res[Iron].stars = 3;
    res[Lead].stars = 4;
    res[Silica].stars = 10;
    res[Aluminium].stars = 4;
    res[Silver].stars = 9;
    res[Gold].stars = 6;
    res[Diamond].stars = 2;
    res[Platinum].stars = 4;
    res[Titanium].stars = 3;
    res[Iridium].stars = 1;
    res[Paladium].stars = 2;
    res[Osmium].stars = 1;

    res[CopperBar].stars = 1;
    res[IronBar].stars = 6;
    res[LeadBar].stars = 2;
    res[SilicaBar].stars = 3;
    res[AluminiumBar].stars = 2;
    res[SilverBar].stars = 4;
    res[GoldBar].stars = 6;
    res[BronzeBar].stars = 5;
    res[SteelBar].stars = 6;
    res[PlatinumBar].stars = 3;
    res[TitaniumBar].stars = 4;
    res[IridiumBar].stars = 2;
    res[PaladiumBar].stars = 3;
    res[OsmiumBar].stars = 3;

    res[CopperWire].stars = 4;
    res[IronNails].stars = 2;
    res[Battery].stars = 2;
    res[Hammer].stars = 3;
    res[Glass].stars = 6;
    res[Circuit].stars = 2;
    res[Lense].stars = 3;
    res[Laser].stars = 3;
    res[BasicComputer].stars = 5;
    res[SolarPanel].stars = 1;
    res[LaserTorch].stars = 2;
    res[AdvancedBattery].stars = 1;
    res[ThermalScanner].stars = 7;
    res[AdvancedComputer].stars = 5;
    res[NavigationModule].stars = 4;
    res[Motor].stars = 4;
    res[NuclearCapsule].stars = 2;

    // multipliers
    //res[Lead].price_multiplier = 0.33;
    //res[AdvancedComputer].price_multiplier = 3;

    return res;
}

const std::array<Resource, ResourceSize> resources = init_resources();

const Resource& res(ResourceId id) {
    return resources[id];
}

double price(Resource r, bool ignore_stars = false, bool ignore_multiplier = false) {
    double res = r.sell_price;
    if (!ignore_stars) res *= 1.0 + r.stars * 0.2;
    if (!ignore_multiplier) res *= r.price_multiplier;
    return res;
}

double price(ResourceId id, bool ignore_stars = false, bool ignore_multiplier = false) {
    return price(res(id), ignore_stars, ignore_multiplier);
}

double price(const ResCount& r, bool ignore_stars = false, bool ignore_multiplier = false) {
    return r.count * price(r.id, ignore_stars, ignore_multiplier);
}

double added_value_per_min(const Resource& r, bool ignore_stars = false, bool ignore_multiplier = false) {
    double val = price(r, ignore_stars, ignore_multiplier);
    for (auto& req : r.required_resources)
        val -= price(req, ignore_stars, ignore_multiplier);
    return val / (r.smelt_time + r.craft_time).count() * 60;
}

double full_time(const Resource& r) {
    double result = (double)(r.craft_time + r.smelt_time).count();
    double mult = r.required_resources_multiplier();
    for (auto req : r.required_resources) {
        req = count_ingredients(req, mult);
        result += full_time(res(req.id)) * req.count;
    }
    return result;
}

double full_craft_time(const Resource& r) {
    double result = (double)r.craft_time.count();
    double mult = r.required_resources_multiplier();
    for (auto& req : r.required_resources)
        result += full_craft_time(res(req.id)) * req.count * mult;
    return result;
}

double full_smelt_time(const Resource& r) {
    double result = (double)r.smelt_time.count();
    double mult = r.required_resources_multiplier();
    for (auto req : r.required_resources) {
        req = count_ingredients(req, mult);
        result += full_smelt_time(res(req.id)) * req.count;
    }
    return result;
}

double ore_value(const Resource& r, bool ignore_stars = false, bool ignore_multiplier = false) {
    if (r.is_ore) return price(r, ignore_stars, ignore_multiplier);
    double mult = r.required_resources_multiplier();
    auto result = 0.0;
    for (auto rc : r.required_resources) {
        rc = count_ingredients(rc, mult);
        result += ore_value(res(rc.id), ignore_stars, ignore_multiplier) * rc.count;
    }
    return result;
}

double full_value_per_hour(const Resource& r) {
    auto st = full_smelt_time(r);
    auto ct = full_craft_time(r);
    return -(ore_value(r) - price(r)) / (st + ct) * 60 * 60 / 1000;
}

void flatten_to(ResCount rc, vector<ResCount>& result) {
    result.push_back(rc);
    auto& r = res(rc.id);
    double m = r.required_resources_multiplier();
    for (auto ingredient : r.required_resources) {
        ingredient = count_ingredients(ingredient, m);
        ingredient.count *= rc.count;
        flatten_to( ingredient, result);
    }
}

void merge_similar(vector<ResCount>& v) {
    sort(rbegin(v), rend(v));
    for (auto p = begin(v); p != end(v); p++) 
        for (auto q = p + 1; q != end(v); ++q) 
            if (p->id == q->id) 
                p->count += exchange(q->count, 0);
        
    auto new_end = remove_if(begin(v), end(v), [](ResCount& rc) {return rc.count == 0; });
    v.erase(new_end, end(v));
}

vector<ResCount> flatten(const Resource& r) {
    vector<ResCount> res;
    flatten_to({ r.id, 1 }, res);
    merge_similar(res);
    return res;
}

bool is_available(const Resource& r, ResourceId best_ore) {
    if (r.is_ore) return r.id <= best_ore;
    for (auto [id, count] : r.required_resources)
        if (!is_available(res(id), best_ore)) return false;
    return true;
}

double calc_ore_value(vector<ResCount>& list) {
    double sum = 0;
    for (auto [id, count] : list) {
        auto& r = res(id);
        if (r.is_ore) sum += count * price(r);
    }
    return sum;
}

void print_detailed_stat(const Resource& r) {
    auto list = flatten(r);

    double st = 0;
    double ct = 0;
    for (auto [id, count] : list) {
        auto& r = res(id);
        st += r.smelt_time.count() * count;
        ct += r.craft_time.count() * count;
    }
    auto ft = st + ct;

    auto ore_value = calc_ore_value(list);
    auto added_value = price(r) - ore_value;
    auto added_value_per_hour = added_value / ft * 3600 / 1000000;

    cout << format("--- {:>16} : {:8.0f} done in {:.1f} min", r.name(), added_value_per_hour, ft / 60);
    if (ct > 0) std::cout << format(" {:.0f}%/{:.0f}%", st * 100 / ft, ct * 100 / ft);
    cout << endl;

    for (auto [id, count] : list) {
        auto& r = res(id);
        auto f = r.is_ore ? "{:>17}    : {:7}" : "{:>20} : {:4}";
        cout << format(f, r.name(), count);
        if (r.is_alloy) cout << format(" smelt {:4.1f}%", count * r.smelt_time.count() / st * 100);
        if (r.is_item) cout << format(" craft {:4.1f}%", count * r.craft_time.count() / ct * 100);
        cout << "\n";
    }
}

void print_detailed_stat(ResourceId id) {
    print_detailed_stat(res(id));
}

int main()
{
    // print_detailed_stat(NavigationModule);

    size_t max_items = 112;

    vector<pair<double, ResourceId>> sorted_alloys;
    for (auto& r : resources) {
        if (r.is_alloy && r.id <= best_alloy && is_available(r, best_ore)) {
            sorted_alloys.emplace_back(full_value_per_hour(r), r.id);
        }
    }
    sort(rbegin(sorted_alloys), rend(sorted_alloys));

    sorted_alloys.resize(min(sorted_alloys.size(), max_items));

    vector<pair<double, ResourceId>> sorted_items;
    for (auto& r : resources) {
        if (r.is_item && r.id <= best_item && is_available(r, best_ore)) {
            sorted_items.emplace_back(full_value_per_hour(r), r.id);
        }
    }
    sort(rbegin(sorted_items), rend(sorted_items));

    sorted_items.resize(min(sorted_items.size(), max_items));

    for (auto& [name, list] : { make_pair("Alloys", sorted_alloys), make_pair("Items", sorted_items) })
    {
        cout << format("\n{}:\n", name);
        for (auto [v, id] : list) {
            auto& r = res(id);
            auto st = full_smelt_time(r);
            auto ct = full_craft_time(r);
            auto ft = st + ct;
            cout << format("--- {:>16} : {:8.0f} done in {:.1f} min", r.name(), v, ft / 60);
            if (ct > 0) std::cout << format(" {:.0f}%/{:.0f}% craft production: {:8.0f}", st * 100 / ft, ct * 100 / ft, v * ft / ct * 60);
            cout << endl;


            auto list = flatten(r);
            for (const auto& i : list) {
                auto& r2 = res(i.id);
                auto f = r2.is_ore ? "{:>17}    : {:7}" : "{:>20} : {:4}";
                cout << format(f, r2.name(), i.count);
                if (r2.is_alloy) cout << format(" smelt {:4.1f}%", i.count * r2.smelt_time.count() / st * 100);
                if (r2.is_item) cout << format(" craft {:4.1f}%", i.count * r2.craft_time.count() / ct * 100);
                cout << "\n";
            }
        }
    }
}
