// SPDX-License-Identifier: BSD-3-Clause
//==============================================================================
//  C++ standard library
//==============================================================================
#include <array>
#include <chrono>
#include <iostream>
//==============================================================================
//  External libraries
//==============================================================================
#include <gtest/gtest.h>
//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/layered.hpp>
TEST(LayeredQueriesPerf, HundredThousandCells) {
#ifndef NDEBUG
    GTEST_SKIP() << "Release timing benchmark";
#else
    constexpr std::size_t columns=10000;
    vmm::MeshData<2> data;
    for(std::size_t i=0;i<=columns;++i) {
        data.points.push_back({static_cast<double>(i),0});
        data.points.push_back({static_cast<double>(i),1});
    }
    const auto edge=[&](std::size_t a,std::size_t b,std::size_t owner) {
        const vmm::VertexId vertices[]={vmm::VertexId::from_index(a),vmm::VertexId::from_index(b)};
        data.face_vertices.push_row(vertices);data.owner.push_back(vmm::CellId::from_index(owner));
    };
    for(std::size_t i=1;i<columns;++i) {
        edge(2*i,2*i+1,i-1);data.neighbour.push_back(vmm::CellId::from_index(i));
    }
    const auto first_boundary=data.owner.size();
    edge(1,0,0);edge(2*columns,2*columns+1,columns-1);
    for(std::size_t i=0;i<columns;++i) {
        edge(2*i,2*(i+1),i);edge(2*(i+1)+1,2*i+1,i);
        data.sites.push_back({static_cast<double>(i)+.5,.5});
        data.cell_region.push_back(vmm::RegionId::from_index(0));
        data.cell_input_site.push_back(vmm::SiteId::from_index(i));
    }
    data.patches={{"wall",first_boundary,data.owner.size()-first_boundary}};
    data.regions={{"r",vmm::MediumId::from_index(0)}};data.media={"m"};
    const auto base=vmm::Mesh2D::from_data(std::move(data)); ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,static_cast<double>(columns)},{0,1},
        {"bottom","top"},{{0,0,0,0},{10,10,10,10}}); ASSERT_TRUE(grid);
    std::vector<double> fractions;for(std::size_t k=0;k<=10;++k) fractions.push_back(static_cast<double>(k)/10);
    const auto mesh=vmm::generate_layered_mesh(*base,*grid,{fractions},vmm::FlatColumns<>{});
    ASSERT_TRUE(mesh) << mesh.error().message();ASSERT_EQ(mesh->mesh().cell_count(),100000u);
    std::size_t cells=0,neighbours=0;
    const auto start=std::chrono::steady_clock::now();
    for(std::size_t c=0;c<columns;++c) for(auto cell:mesh->cells_in_column_view(c)) {
        ++cells;neighbours+=mesh->vertical_neighbours_view(cell).size();
    }
    const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::cout << "columns=" << columns << " cells=" << cells << " query_ms=" << ms << '\n';
    EXPECT_EQ(cells,100000u);EXPECT_EQ(neighbours,180000u);EXPECT_LT(ms,50.);
#endif
}

TEST(LayeredQueriesPerf, FourfoldColumnsStayBelowSixfoldTime) {
#ifndef NDEBUG
    GTEST_SKIP() << "Release timing benchmark";
#else
    std::array<double,2> durations{};
    for(std::size_t run=0;run<2;++run) {
    const std::size_t columns=run==0?10000:40000;
    vmm::MeshData<2> data;
    for(std::size_t i=0;i<=columns;++i) {
        data.points.push_back({static_cast<double>(i),0});
        data.points.push_back({static_cast<double>(i),1});
    }
    const auto edge=[&](std::size_t a,std::size_t b,std::size_t owner) {
        const vmm::VertexId vertices[]={vmm::VertexId::from_index(a),vmm::VertexId::from_index(b)};
        data.face_vertices.push_row(vertices);data.owner.push_back(vmm::CellId::from_index(owner));
    };
    for(std::size_t i=1;i<columns;++i) {
        edge(2*i,2*i+1,i-1);data.neighbour.push_back(vmm::CellId::from_index(i));
    }
    const auto first_boundary=data.owner.size();
    edge(1,0,0);edge(2*columns,2*columns+1,columns-1);
    for(std::size_t i=0;i<columns;++i) {
        edge(2*i,2*(i+1),i);edge(2*(i+1)+1,2*i+1,i);
        data.sites.push_back({static_cast<double>(i)+.5,.5});
        data.cell_region.push_back(vmm::RegionId::from_index(0));
        data.cell_input_site.push_back(vmm::SiteId::from_index(i));
    }
    data.patches={{"wall",first_boundary,data.owner.size()-first_boundary}};
    data.regions={{"r",vmm::MediumId::from_index(0)}};data.media={"m"};
    const auto base=vmm::Mesh2D::from_data(std::move(data)); ASSERT_TRUE(base);
    const auto grid=vmm::HorizonGrid::make({0,static_cast<double>(columns)},{0,1},
        {"bottom","top"},{{0,0,0,0},{10,10,10,10}}); ASSERT_TRUE(grid);
    std::vector<double> fractions;for(std::size_t k=0;k<=10;++k) fractions.push_back(static_cast<double>(k)/10);
    const auto generation_start=std::chrono::steady_clock::now();
    const auto mesh=vmm::generate_layered_mesh(*base,*grid,{fractions},vmm::FlatColumns<>{});
    durations[run]=std::chrono::duration<double>(std::chrono::steady_clock::now()-generation_start).count();
    ASSERT_TRUE(mesh) << mesh.error().message();ASSERT_EQ(mesh->mesh().cell_count(),columns*10);
    std::size_t cells=0,neighbours=0;
    const auto start=std::chrono::steady_clock::now();
    for(std::size_t c=0;c<columns;++c) for(auto cell:mesh->cells_in_column_view(c)) {
        ++cells;neighbours+=mesh->vertical_neighbours_view(cell).size();
    }
    const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::cout << "columns=" << columns << " cells=" << cells << " query_ms=" << ms << '\n';
    EXPECT_EQ(cells,columns*10);EXPECT_EQ(neighbours,columns*18);EXPECT_LT(ms,50.);
    }
    std::cout << "generation_s_10000=" << durations[0] << " generation_s_40000=" << durations[1] << " ratio=" << durations[1]/durations[0] << '\n';
    EXPECT_LT(durations[1],6*durations[0]);
#endif
}
