#include "shapes.hpp"

#include <numbers>

// Derived from exercise 4 - authors @Mayur Shankar and @Jose Vaz
SimpleMeshData make_cylinder( bool aCapped, std::size_t aSubdivs, Vec3f aColor, Mat44f aPreTransform )
{
  	std::vector< Vec3f > pos;
    std::vector< Vec3f > normal;
    float prevY = std::cos(0.f);
    float prevZ = std::sin(0.f);

    // Pre-compute normal
    Mat33f const N = mat44_to_mat33 ( transpose (invert ( aPreTransform ) ) );

    // aSubDivs refers to how many segments the cylinder contains
    for (std::size_t i = 0; i < aSubdivs; ++i) {
        float angle = (i + 1) * 2.0f * std::numbers::pi_v<float> / float(aSubdivs);
        float y = std::cos(angle);
        float z = std::sin(angle);

        // Rectangle containing first triangle
        pos.emplace_back(Vec3f{ 0.f, prevY, prevZ });
        pos.emplace_back(Vec3f{ 0.f, y, z });
        pos.emplace_back(Vec3f{ 1.f, prevY, prevZ });

        // Rectangle containing second triangle
        pos.emplace_back(Vec3f{ 0.f, y, z });
        pos.emplace_back(Vec3f{ 1.f, y, z });
        pos.emplace_back(Vec3f{ 1.f, prevY, prevZ });

        // Normals for first triangle
        normal.emplace_back(Vec3f{ 0.f, prevY, prevZ});
        normal.emplace_back(Vec3f{ 0.f, y, z});
        normal.emplace_back(Vec3f{ 0.f, prevY, prevZ});

		// Normal for second triangle
        normal.emplace_back(Vec3f{ 0.f, y, z});
        normal.emplace_back(Vec3f{ 0.f, y, z});
        normal.emplace_back(Vec3f{ 0.f, prevY, prevZ});

        // Increment to next rectangle
        prevY = y;
        prevZ = z;
    }

    // For capped cylinders
    if (aCapped)
    {
        Vec3f topCenter{ 1.f, 0.f, 0.f };
        Vec3f bottomCenter{ 0.f, 0.f, 0.f };

        for (std::size_t i = 0; i < aSubdivs; ++i)
        {
            float angle = (i+1) * 2.0f * std::numbers::pi_v<float> / float(aSubdivs);
            float y = std::cos(angle), z = std::sin(angle);

            // First triangle
            pos.emplace_back(topCenter);
            pos.emplace_back(Vec3f{ 1.f, y, z });
            pos.emplace_back(Vec3f{ 1.f, prevY, prevZ });

            // Second triangle
            pos.emplace_back(bottomCenter);
            pos.emplace_back(Vec3f{ 0.f, prevY, prevZ });
            pos.emplace_back(Vec3f{ 0.f, y, z });

            /// Normal for first triangle
            normal.emplace_back(topCenter);
            normal.emplace_back(topCenter);
            normal.emplace_back(topCenter);

            // Normal for second triangle
            normal.emplace_back(Vec3f{-1.f, 0.f, 0.f});
            normal.emplace_back(Vec3f{-1.f, 0.f, 0.f});
            normal.emplace_back(Vec3f{-1.f, 0.f, 0.f});

            // Next triangles
            prevY = y;
            prevZ = z;
        }
    }

    // Apply transformation
    for (auto& p : pos) {
        Vec4f p4{p.x, p.y, p.z, 1.f};
        Vec4f t = aPreTransform * p4;
        t /= t.w;

        p = Vec3f{t.x, t.y, t.z};
    }

    // Apply normals
    for (auto& n : normal) {
      Vec3f z = N * n;
      n = Vec3f{ z.x, z.y, z.z };
    }

    // Use inputted colour
    std::vector col (pos.size(), aColor);

    return SimpleMeshData{ std::move(pos), std::move(col), std::move(normal) };
}


// Derived from exercise 4 - authors @Mayur Shankar and @Jose Vaz
SimpleMeshData make_cone(bool aCapped, std::size_t aSubdivs, Vec3f aColor, Mat44f aPreTransform)
{
    std::vector<Vec3f> pos;
    std::vector< Vec3f > normal;

    pos.reserve(aSubdivs * 3 + (aCapped ? aSubdivs * 3 : 0));

    // Pre-compute normal
    Mat33f const N = mat44_to_mat33 ( transpose (invert ( aPreTransform ) ) );

    // Apex of the cone
    Vec3f apex = { 1.f, 0.f, 0.f };

    // Start angle
    float prevY = std::cos(0.f);
    float prevZ = std::sin(0.f);

    for (std::size_t i = 0; i < aSubdivs; ++i)
    {
        float angle = (float(i + 1) / float(aSubdivs)) * 2.f * std::numbers::pi_v<float>;
        float y = std::cos(angle);
        float z = std::sin(angle);

        // Add triangles - only one set needed
        pos.push_back(apex);
        pos.push_back(Vec3f{ 0.f, prevY, prevZ });
        pos.push_back(Vec3f{ 0.f, y, z });

        // Add normals
        normal.emplace_back(apex);
        normal.emplace_back(Vec3f{ 0.f, prevY, prevZ });
        normal.emplace_back(Vec3f{ 0.f, y, z });

        // If capped, create the cap at x=0:
        if (aCapped)
        {

            // Add triangles
            pos.push_back(Vec3f{ 0.f, 0.f, 0.f });
            pos.push_back(Vec3f{ 0.f, y, z });
            pos.push_back(Vec3f{ 0.f, prevY, prevZ });

            // Add normals
            normal.emplace_back(Vec3f{ 0.f, 0.f, 0.f });
            normal.emplace_back(Vec3f{ 0.f, y, z });
            normal.emplace_back(Vec3f{ 0.f, prevY, prevZ });
        }

        // Next triangle
        prevY = y;
        prevZ = z;
    }

    // Apply transformation
    for (auto& p : pos)
    {
        Vec4f p4{ p.x, p.y, p.z, 1.f };
        Vec4f t = aPreTransform * p4;
        t /= t.w;
        p = Vec3f{ t.x, t.y, t.z };
    }

    // Apply normals
    for (auto& n: normal) {
      Vec3f z = N * n;
      n = Vec3f{ z.x, z.y, z.z };
	}

    // Use inputted colour
    std::vector<Vec3f> colors (pos.size(), aColor);

    return SimpleMeshData{ std::move(pos), std::move(colors), std::move(normal) };
}

// Derived from exercise 3 - authors @Mayur Shankar and @Jose Vaz
SimpleMeshData make_cube(bool aCapped, std::size_t aSubdivs, Vec3f aColor, Mat44f aPreTransform)
{
    std::vector<Vec3f> pos;
    std::vector<Vec3f> normal;

    // Pre-compute normal
    Mat33f const N = mat44_to_mat33 ( transpose (invert ( aPreTransform ) ) );

    // 36 vertices - 6 * 6 since each face is made up of two triangles
    for (std::size_t i = 0; i < 36; i++)
    {
     	// Add triangle
        pos.push_back(Vec3f{kCubePositions[i*3], kCubePositions[i*3+1], kCubePositions[i*3+2]});

        // Add normal
        normal.push_back(Vec3f{kCubePositions[i*3], kCubePositions[i*3+1], kCubePositions[i*3+2]});
    }

    // Apply the pre-transform
    for (auto& p : pos)
    {
        Vec4f p4{ p.x, p.y, p.z, 1.f };
        Vec4f t = aPreTransform * p4;
        t /= t.w;
        p = Vec3f{ t.x, t.y, t.z };
    }

    // Apply normal
    for (auto& n: normal) {
      Vec3f z = N * n;
      n = Vec3f{ z.x, z.y, z.z };
	}

    // Use inputted colour
    std::vector<Vec3f> colors (pos.size(), aColor);

    return SimpleMeshData{ std::move(pos), std::move(colors), std::move(normal) };
}

// Author @Jose Vaz
SimpleMeshData make_particle_quad(const std::vector<Vec3f>& particleCenters, const Vec3f& camRight, const Vec3f& camUp, float size)
{
    SimpleMeshData mesh;
    for (const auto& centre : particleCenters)
    {
        Vec3f right = camRight * size;
        Vec3f up = camUp * size;

        // First triangle
        mesh.positions.emplace_back(centre - right - up);
        mesh.positions.emplace_back(centre + right - up);
        mesh.positions.emplace_back(centre + right + up);

        mesh.texcoords.emplace_back(Vec2f{ 0.f, 0.f });
        mesh.texcoords.emplace_back(Vec2f{ 1.f, 0.f });
        mesh.texcoords.emplace_back(Vec2f{ 1.f, 1.f });

        // Second triangle
        mesh.positions.emplace_back(centre - right - up);
        mesh.positions.emplace_back(centre + right + up);
        mesh.positions.emplace_back(centre - right + up);

        mesh.texcoords.emplace_back(Vec2f{ 0.f, 0.f });
        mesh.texcoords.emplace_back(Vec2f{ 1.f, 1.f });
        mesh.texcoords.emplace_back(Vec2f{ 0.f, 1.f });
    }
    return mesh;
}
