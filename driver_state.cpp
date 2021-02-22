#include "driver_state.h"
#include <cstring>

driver_state::driver_state()
{
}

driver_state::~driver_state()
{
    delete [] image_color;
    delete [] image_depth;
}

// This function should allocate and initialize the arrays that store color and
// depth.  This is not done during the constructor since the width and height
// are not known when this class is constructed.
void initialize_render(driver_state& state, int width, int height)
{
    state.image_width=width;
    state.image_height=height;
    state.image_color=0;
    state.image_depth=0;

    state.image_color=new pixel[width * height];
    state.image_depth = new float[width * height];
    for(int i = 0; i < width * height; i++) {
        state.image_color[i] = make_pixel(0,0,0);
        state.image_depth[i] = 1; //Generic number, don't need to do anything with this yet
    }
    //std::cout<<"TODO: allocate and initialize state.image_color and state.image_depth."<<std::endl;
}

// This function will be called to render the data that has been stored in this class.
// Valid values of type are:
//   render_type::triangle - Each group of three vertices corresponds to a triangle.
//   render_type::indexed -  Each group of three indices in index_data corresponds
//                           to a triangle.  These numbers are indices into vertex_data.
//   render_type::fan -      The vertices are to be interpreted as a triangle fan.
//   render_type::strip -    The vertices are to be interpreted as a triangle strip.
void render(driver_state& state, render_type type)
{
    bool myDebug = false;
    const data_geometry* triangleVertices[3];
    data_geometry dgPasser[3];
    data_vertex dvPasser[3];
    int vertexEntry = 0; //Traverses to the beginning of each set of floats which make up a vertex

    switch(type) {
        case render_type::triangle: {
          if (myDebug) {std::cout << "state.num_vertices: " << state.num_vertices << "\n";
          std::cout << "state.floats_per_vertex: " << state.floats_per_vertex << "\n";}

            for (int i = 0; i < state.num_vertices; i+= 3) { //Triangles must have 3 vertices
                for (int j = 0; j < 3; j++) { //Go through the XYZ values in the vertex
                    dvPasser[j].data = &state.vertex_data[vertexEntry]; //Get the address of the float value which marks the start of the vertex entry
                    dgPasser[j].data = &state.vertex_data[vertexEntry];
                    state.vertex_shader(dvPasser[j], dgPasser[j], state.uniform_data);
                    triangleVertices[j] = &dgPasser[j]; //Store the current vertex so that it can be processed- the next set of vertices overwrites this one
                    vertexEntry = vertexEntry + state.floats_per_vertex;

                    if (myDebug) {
                        for (int db = 0; db < state.floats_per_vertex; db++) { //DEBUGGING
                            std::cout << *(triangleVertices[j]->data + db) << "\n";
                        }
                        std::cout << "Vertex \"" << j << "\" processed.\n\n";
                        std::cout << "END OF void render() DEBUG\n\n";
                    }

                }
                rasterize_triangle(state, *triangleVertices[0], *triangleVertices[1], *triangleVertices[2]);
            }

        }
        case render_type::fan: {}
        case render_type::indexed: {}
        case render_type::invalid: {}
        case render_type::strip: {}
    }

   // std::cout<<"TODO: implement rendering."<<std::endl;
}


// This function clips a triangle (defined by the three vertices in the "in" array).
// It will be called recursively, once for each clipping face (face=0, 1, ..., 5) to
// clip against each of the clipping faces in turn.  When face=6, clip_triangle should
// simply pass the call on to rasterize_triangle.
void clip_triangle(driver_state& state, const data_geometry& v0,
    const data_geometry& v1, const data_geometry& v2,int face)
{
    if(face==6)
    {
        rasterize_triangle(state, v0, v1, v2);
        return;
    }
    //std::cout<<"TODO: implement clipping. (The current code passes the triangle through without clipping them.)"<<std::endl;
    clip_triangle(state, v0, v1, v2,face+1);
}

// Rasterize the triangle defined by the three vertices in the "in" array.  This
// function is responsible for rasterization, interpolation of data to
// fragments, calling the fragment shader, and z-buffering.
void rasterize_triangle(driver_state& state, const data_geometry& v0,
    const data_geometry& v1, const data_geometry& v2)
{
    bool myDebug = false;


    if (myDebug) {
        std::cout << "rasterize_triangle() called.\n";

        int vertex_X;
        int vertex_Y;
        //Draw the vertices
        vertex_X = static_cast<int>((state.image_width / 2.0) * v0.gl_Position[0] + ((state.image_width / 2.0) - 0.5));
        vertex_Y = static_cast<int>((state.image_height / 2.0) * v0.gl_Position[1] +
                                    ((state.image_height / 2.0) - 0.5));
        state.image_color[vertex_X + vertex_Y * state.image_width] = make_pixel(255, 255, 255);
        if (myDebug) { std::cout << "vertex_X:  " << vertex_X << "  vertex_Y:  " << vertex_Y << "\n"; }

        vertex_X = static_cast<int>((state.image_width / 2.0) * v1.gl_Position[0] + ((state.image_width / 2.0) - 0.5));
        vertex_Y = static_cast<int>((state.image_height / 2.0) * v1.gl_Position[1] +
                                    ((state.image_height / 2.0) - 0.5));
        state.image_color[vertex_X + vertex_Y * state.image_width] = make_pixel(255, 255, 255);
        if (myDebug) { std::cout << "vertex_X:  " << vertex_X << "  vertex_Y:  " << vertex_Y << "\n"; }

        vertex_X = static_cast<int>((state.image_width / 2.0) * v2.gl_Position[0] + ((state.image_width / 2.0) - 0.5));
        vertex_Y = static_cast<int>((state.image_height / 2.0) * v2.gl_Position[1] +
                                    ((state.image_height / 2.0) - 0.5));
        state.image_color[vertex_X + vertex_Y * state.image_width] = make_pixel(255, 255, 255);
        if (myDebug) { std::cout << "vertex_X:  " << vertex_X << "  vertex_Y:  " << vertex_Y << "\n"; }
    }


    float totalArea;
    float ALPHA;
    float BETA;
    float GAMMA;
    //Save the vertices as ABC so the formulas are easier
    int A[2];
    int B[2];
    int C[2];

    A[0] = static_cast<int>((state.image_width / 2.0) * v0.gl_Position[0] + ((state.image_width / 2.0) - 0.5));
    A[1] = static_cast<int>((state.image_height / 2.0) * v0.gl_Position[1] + ((state.image_height / 2.0) - 0.5));

    B[0] = static_cast<int>((state.image_width / 2.0) * v1.gl_Position[0] + ((state.image_width / 2.0) - 0.5));
    B[1] = static_cast<int>((state.image_height / 2.0) * v1.gl_Position[1] + ((state.image_height / 2.0) - 0.5));

    C[0] = static_cast<int>((state.image_width / 2.0) * v2.gl_Position[0] + ((state.image_width / 2.0) - 0.5));
    C[1] = static_cast<int>((state.image_height / 2.0) * v2.gl_Position[1] + ((state.image_height / 2.0) - 0.5));



    //AREA(ABC) = 0.5((BXCY-CXBY)+(CXAY-AXCY)+(AXBY-BXAY))
    totalArea = (
              (B[0]*C[1] - C[0]*B[1])
            + (C[0]*A[1]-A[0]*C[1])
            + (A[0]*B[1] - B[0]*A[1])
            )/2;

    if (myDebug) { std::cout << "Total Area: " << totalArea << "\n";}

    for (int i = 0; i < state.image_width; i++) {
        for (int j = 0; j < state.image_height; j++) {
            ALPHA = ((
                      (B[0]*C[1] - C[0]*B[1])
                    + (B[1]-C[1])*i
                    + (C[0]-B[0])*j
                    )/2)/totalArea;

            BETA = ((
                             (C[0]*A[1] - A[0]*C[1])
                             + (C[1]-A[1])*i
                             + (A[0]-C[0])*j
                     )/2)/totalArea;

            GAMMA = ((
                             (A[0]*B[1] - B[0]*A[1])
                             + (A[1]-B[1])*i
                             + (B[0]-A[0])*j
                     )/2)/totalArea;

            if (ALPHA >= 0 && BETA >= 0 && GAMMA >= 0) {

                if (myDebug) {std::cout << "\nAlpha Beta Gamma >= 0 for: " << "      ALPHA: " << ALPHA << " BETA: " << BETA << " GAMMA: " << GAMMA << "\n";
                std::cout << "X: " << i << "   Y: " << j << "\n";}

                state.image_color[j*state.image_width + i] = make_pixel(255,255,255);
            }
        }
    }

    if (myDebug) {std::cout << "state.image_width: " << state.image_width << "    state.image_height: " << state.image_height << "\n";}


    //std::cout<<"TODO: implement rasterization"<<std::endl;
}

